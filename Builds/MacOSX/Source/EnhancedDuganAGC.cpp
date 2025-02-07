// EnhancedDuganAGC.cpp
#include "EnhancedDuganAGC.h"
#include <cmath>
#include <algorithm>
#include "ChannelStripComponent.h"
// Convert decibels to linear scale:
float EnhancedDuganAGC::dbToLinear(float dB)
{
    return std::pow(10.f, dB / 20.f);
}

// Convert linear scale to decibels:
float EnhancedDuganAGC::linearToDb(float lin)
{
    if (lin < 1.0e-9f) return -90.f;
    return 20.f * std::log10(lin);
}

void EnhancedDuganAGC::prepare(double sampleRate, int blkSize, int mainChannels, int sideChainCount)
{
    sr = sampleRate;
    blockSize = blkSize;
    numCh = mainChannels;
    sideCh = sideChainCount;

    channels.clear();
    channels.resize(numCh);

    // Set up lookahead buffer:
    float laMsVal = lookaheadMs.load();
    int laSamples = static_cast<int>(std::ceil((laMsVal / 1000.f) * sr)) + blkSize;
    lookaheadBufferSize = std::max(laSamples, blkSize * 2);
    lookaheadBuffer.resize(numCh * lookaheadBufferSize, 0.f);
    writePos = 0;

    lastActiveChannel = 0;
}

void EnhancedDuganAGC::setLookaheadMs(float ms)
{
    lookaheadMs.store(ms);
    int laSamples = static_cast<int>(std::ceil((ms / 1000.f) * sr)) + blockSize;
    lookaheadBufferSize = std::max(laSamples, blockSize * 2);
    lookaheadBuffer.resize(numCh * lookaheadBufferSize, 0.f);
    writePos = 0;
}

void EnhancedDuganAGC::processBlock(float** mainData, int mainCh, int numSamples,
                                    float** sideData, int sideCh, int sideSamples)
{
    if (mainCh != numCh)
        return;
    processBlockInternal(mainData, mainCh, numSamples, sideData, sideCh, sideSamples);
}

void EnhancedDuganAGC::processBlockInternal(float** audioData, int nChannels, int nSamples,
                                            float** sideData, int sideChs, int sideSamples)
{
    // 1) Update ML/VAD states (stubbed):
    if (useMLSpeechDetection.load())
        updateMLSpeechStates();

    // 2) Copy to lookahead buffer:
    float laMsVal = lookaheadMs.load();
    int laSamples = static_cast<int>(std::ceil((laMsVal / 1000.f) * sr));
    laSamples = std::min(laSamples, lookaheadBufferSize - nSamples);

    for (int ch = 0; ch < nChannels; ++ch)
    {
        float* wb = &lookaheadBuffer[ch * lookaheadBufferSize];
        for (int i = 0; i < nSamples; ++i)
            wb[(writePos + i) % lookaheadBufferSize] = audioData[ch][i];
    }
    int readPos = (writePos + lookaheadBufferSize - laSamples) % lookaheadBufferSize;
    writePos = (writePos + nSamples) % lookaheadBufferSize;

    // Local block data for processing:
    std::vector<std::vector<float>> blockData(nChannels);
    for (int ch = 0; ch < nChannels; ++ch)
    {
        blockData[ch].resize(nSamples);
        float* wb = &lookaheadBuffer[ch * lookaheadBufferSize];
        for (int i = 0; i < nSamples; ++i)
            blockData[ch][i] = wb[(readPos + i) % lookaheadBufferSize];
    }

    // 3) Measure sidechain RMS (if provided):
    float sideRmsDb = -90.f;
    if (sideChs > 0 && sideData != nullptr && sideSamples > 0)
    {
        double sumSq = 0.0;
        for (int i = 0; i < sideSamples; ++i)
            sumSq += sideData[0][i] * sideData[0][i];
        float scRms = static_cast<float>(std::sqrt(sumSq / sideSamples));
        sideRmsDb = linearToDb(scRms);
    }

    // 4) Recursive RMS smoothing coefficients:
    float stMsVal = shortTermMs.load();
    float ltMsVal = longTermMs.load();
    double stAlpha = double(nSamples) / ((stMsVal / 1000.0) * sr + 1e-9);
    double ltAlpha = double(nSamples) / ((ltMsVal / 1000.0) * sr + 1e-9);
    float stCoef = static_cast<float>(std::exp(-1.0 / std::max(1.0, stAlpha)));
    float ltCoef = static_cast<float>(std::exp(-1.0 / std::max(1.0, ltAlpha)));

    float baseGateThreshold = gateThreshold.load();
    if (useAdaptiveThreshold.load())
    {
        baseGateThreshold = computeAdaptiveThreshold(baseGateThreshold, sideRmsDb);
    }
    float hyst = gateHysteresis.load();
    float gateOn = baseGateThreshold + hyst;
    float gateOff = baseGateThreshold - hyst;

    // Attack/Release coefficients:
    float attMs = gateAttackMs.load();
    float relMs = gateReleaseMs.load();
    float attCoeff = 1.0f - std::exp(-1.f / (attMs * 0.001f * sr + 1e-9f));
    float relCoeff = 1.0f - std::exp(-1.f / (relMs * 0.001f * sr + 1e-9f));

    bool anyActive = false;
    float loudestDb = -999.f;
    int loudestCh = 0;

    // 5) Process each channel:
    for (int ch = 0; ch < nChannels; ++ch)
    {
        auto& c = channels[ch];
        if (c.mute)
        {
            c.gateActive = false;
            c.finalGain = 0.f;
            continue;
        }
        if (c.bypass || !c.automix)
        {
            double sumSq = 0.0;
            for (int i = 0; i < nSamples; ++i)
                sumSq += blockData[ch][i] * blockData[ch][i];
            float blkRms = static_cast<float>(std::sqrt(sumSq / (nSamples + 1e-9)));
            c.shortTermRMS = stCoef * c.shortTermRMS + (1.f - stCoef) * blkRms;
            c.longTermRMS = ltCoef * c.longTermRMS + (1.f - ltCoef) * blkRms;
            float stDb = linearToDb(blkRms) + c.sensDb + c.faderDb;
            c.finalGain = dbToLinear(stDb) * masterGain.load();
            c.gateActive = false;
            continue;
        }

        // Compute RMS and update smoothing:
        double sumSq = 0.0;
        for (int i = 0; i < nSamples; ++i)
            sumSq += blockData[ch][i] * blockData[ch][i];
        float blkRms = static_cast<float>(std::sqrt(sumSq / (nSamples + 1e-9)));
        c.shortTermRMS = stCoef * c.shortTermRMS + (1.f - stCoef) * blkRms;
        c.longTermRMS = ltCoef * c.longTermRMS + (1.f - ltCoef) * blkRms;

        // Use ML/VAD state if enabled:
        bool mlOk = true;
        if (useMLSpeechDetection.load() && (ch < 32))
            mlOk = mlSpeechActiveForChannel[ch];

        float stDb = linearToDb(c.shortTermRMS) + c.sensDb;
        bool wasActive = c.gateActive;
        bool wantOpen = false;
        if (wasActive)
        {
            if (stDb > gateOff && mlOk)
                wantOpen = true;
        }
        else
        {
            if (stDb > gateOn && mlOk)
                wantOpen = true;
        }

        float target = wantOpen ? 1.f : 0.f;
        float coeff = wantOpen ? attCoeff : relCoeff;
        c.gateEnv = c.gateEnv + coeff * (target - c.gateEnv);
        c.gateActive = (c.gateEnv > 0.5f);

        if (c.gateActive)
        {
            anyActive = true;
            if (stDb > loudestDb)
            {
                loudestDb = stDb;
                loudestCh = ch;
            }
        }
    }

    // 6) Last mic on logic:
    if (!anyActive && lastMicOn.load())
    {
        channels[lastActiveChannel].gateActive = true;
        channels[lastActiveChannel].gateEnv = 1.f;
    }
    else if (anyActive)
    {
        lastActiveChannel = loudestCh;
    }

    // 7) Gain sharing:
    double sumActive = 0.0;
    for (int ch = 0; ch < nChannels; ++ch)
    {
        auto& c = channels[ch];
        if (c.mute || c.bypass || !c.automix)
            continue;
        if (c.gateActive)
        {
            float stDb = linearToDb(c.shortTermRMS + 1e-9f);
            float lin = dbToLinear(stDb);
            sumActive += lin;
        }
    }
    for (int ch = 0; ch < nChannels; ++ch)
    {
        auto& c = channels[ch];
        if (c.mute || c.bypass || !c.automix)
            continue;
        if (!c.gateActive)
        {
            c.finalGain = dbToLinear(gateCloseDb.load()) * c.gateEnv;
        }
        else
        {
            float stDb = linearToDb(c.shortTermRMS + 1e-9f);
            float lin = dbToLinear(stDb);
            if (sumActive < 1e-9)
                c.finalGain = 1.f / nChannels;
            else
                c.finalGain = lin / static_cast<float>(sumActive);
        }
        c.finalGain *= dbToLinear(c.faderDb);
        c.finalGain *= masterGain.load();
    }

    // 8) Apply leveler if needed:
    if (linkLeveler.load())
    {
        float maxLin = dbToLinear(levelerRangeDb.load());
        for (auto& c : channels)
        {
            if (c.finalGain > maxLin)
                c.finalGain = maxLin;
        }
    }

    // 9) Write final gain to audio output:
    for (int ch = 0; ch < nChannels; ++ch)
    {
        float g = channels[ch].finalGain;
        for (int i = 0; i < nSamples; ++i)
            audioData[ch][i] *= g;
    }
}

float EnhancedDuganAGC::computeAdaptiveThreshold(float baseThresh, float sidechainDb)
{
    float infl = sidechainInfluence.load();
    float offset = infl * sidechainDb * 0.5f / 40.f;
    return baseThresh + offset;
}

void EnhancedDuganAGC::setChannelMute(int ch, bool b)
{
    if (ch >= 0 && ch < static_cast<int>(channels.size()))
        channels[ch].mute = b;
}

void EnhancedDuganAGC::setChannelBypass(int ch, bool b)
{
    if (ch >= 0 && ch < static_cast<int>(channels.size()))
        channels[ch].bypass = b;
}

void EnhancedDuganAGC::setChannelAutomixOn(int ch, bool b)
{
    if (ch >= 0 && ch < static_cast<int>(channels.size()))
        channels[ch].automix = b;
}

void EnhancedDuganAGC::setChannelSensDb(int ch, float dB)
{
    if (ch >= 0 && ch < static_cast<int>(channels.size()))
        channels[ch].sensDb = dB;
}

void EnhancedDuganAGC::setChannelFaderDb(int ch, float dB)
{
    if (ch >= 0 && ch < static_cast<int>(channels.size()))
        channels[ch].faderDb = dB;
}

float EnhancedDuganAGC::getChannelShortTermRMS(int ch) const
{
    if (ch < 0 || ch >= static_cast<int>(channels.size()))
        return 0.f;
    return channels[ch].shortTermRMS;
}

float EnhancedDuganAGC::getChannelAutoGainDb(int ch) const
{
    if (ch < 0 || ch >= static_cast<int>(channels.size()))
        return 0.f;
    return linearToDb(channels[ch].finalGain);
}

void EnhancedDuganAGC::updateMLSpeechStates()
{
    // This is a stub. In a production system, you would pull from a lock-free FIFO of SpeechResult.
    // For demo purposes, we'll simulate that channel 0 is always active if ML is enabled.
    for (int ch = 0; ch < 32; ++ch)
        mlSpeechActiveForChannel[ch] = (ch == 0);
}
