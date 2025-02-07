#include "MyDuganAutomixer.h"
#include <cmath>
#include <algorithm>
#include "ChannelStripComponent.h"
//static constexpr float kMinDb = -80.f;

inline float MyDuganAutomixer::dbToLin(float dB)
{
    return std::pow(10.f, dB / 20.f);
}

inline float MyDuganAutomixer::linToDb(float lin)
{
    if (lin < 1.0e-9f) return -90.f;
    return 20.f * std::log10(lin);
}

void MyDuganAutomixer::prepare(double sampleRate, int blkSize, int mainChannels, int sideChainChannels)
{
    sr        = sampleRate;
    blockSize = blkSize;
    numCh     = mainChannels;
    sideCh    = sideChainChannels;

    channels.clear();
    channels.resize(numCh); // (Re)allocate channel structs

    // Recompute lookahead buffer
    float laMsVal = lookaheadMs.load();
    int laSamples = (int)std::ceil((laMsVal / 1000.f) * sr) + blockSize;

    // Just in case block sizes grow unexpectedly, give a bit extra
    lookaheadBufferSize = std::max(laSamples, blkSize * 3);
    lookaheadBuffer.resize(numCh * lookaheadBufferSize, 0.f);
    writePos = 0;

    lastActiveChannel = 0;
}

void MyDuganAutomixer::setLookaheadMs(float ms)
{
    lookaheadMs.store(ms);
    // We'll re-allocate in prepareToPlay next time.
    // (Safer than re-allocating ring buffers live.)
}

void MyDuganAutomixer::processBlock(float** mainData, int mainCh, int numSamples,
                                    float** sideData, int sideCh, int sideSamples)
{
    if (mainCh != numCh || mainCh <= 0 || numSamples <= 0)
        return;

    const float sr_ = (float) sr;
    // 1) Write to lookahead ring buffer
    int laSamps = (int)std::ceil((lookaheadMs.load() / 1000.f)*sr_);
    // Make sure we don't overwrite beyond buffer size:
    if (numSamples > lookaheadBufferSize)
        return; // Failsafe: block bigger than ring buffer => skip

    laSamps = std::min(laSamps, lookaheadBufferSize - numSamples);

    for (int ch=0; ch < numCh; ++ch)
    {
        float* wb = &lookaheadBuffer[ch * lookaheadBufferSize];
        for (int i=0; i < numSamples; ++i)
            wb[(writePos + i) % lookaheadBufferSize] = mainData[ch][i];
    }

    int readPos = (writePos + lookaheadBufferSize - laSamps) % lookaheadBufferSize;
    writePos = (writePos + numSamples) % lookaheadBufferSize;

    // Copy data from ring buffer into a local block
    std::vector<std::vector<float>> blockData(numCh);
    for (int ch=0; ch<numCh; ++ch)
    {
        blockData[ch].resize(numSamples);
        float* wb = &lookaheadBuffer[ch * lookaheadBufferSize];
        for (int i=0; i<numSamples; ++i)
            blockData[ch][i] = wb[(readPos + i) % lookaheadBufferSize];
    }

    // 2) Possibly measure sidechain for adaptive threshold
    float sideDb = -90.f;
    if (sideCh > 0 && sideData != nullptr && sideSamples > 0)
    {
        double sumSq = 0.0;
        for (int i=0; i<sideSamples; ++i)
            sumSq += sideData[0][i] * sideData[0][i]; // measure just channel 0
        float scRms = (float)std::sqrt(sumSq / sideSamples);
        sideDb = linToDb(scRms);
    }

    // 3) RMS smoothing
    float stMsVal = shortTermMs.load();
    float ltMsVal = longTermMs.load();

    double stAlpha = double(numSamples) / ((stMsVal / 1000.0) * sr_ + 1e-9);
    double ltAlpha = double(numSamples) / ((ltMsVal / 1000.0) * sr_ + 1e-9);

    float stCoef = (float)std::exp(-1.0 / std::max(1.0, stAlpha));
    float ltCoef = (float)std::exp(-1.0 / std::max(1.0, ltAlpha));

    // gating thresholds
    float gThres = gateThreshold.load();
    if (useAdaptiveThreshold.load())
        gThres = computeAdaptiveThreshold(gThres, sideDb);

    float hyst   = gateHysteresis.load();
    float gateOn  = gThres + hyst;
    float gateOff = gThres - hyst;

    float closeDb = gateCloseDb.load();

    bool anyActive = false;
    float loudestDb = -999.f;
    int loudestCh   = 0;

    // Attack/release
    float attMs = gateAttackMs.load();
    float relMs = gateReleaseMs.load();
    float attCoeff = 1.f - std::exp(-1.f / ((attMs*0.001f*sr_)+1e-9f));
    float relCoeff = 1.f - std::exp(-1.f / ((relMs*0.001f*sr_)+1e-9f));

    // 4) For each channel, measure gating
    for (int ch=0; ch<numCh; ++ch)
    {
        auto& c = channels[ch];
        // If muted => finalGain=0
        if (c.mute)
        {
            c.gateActive = false;
            c.finalGain  = 0.f;
            // still measure RMS for UI
            double sumSq=0.0;
            for (int i=0; i<numSamples; ++i)
                sumSq += blockData[ch][i] * blockData[ch][i];
            float blkRms = (float)std::sqrt(sumSq / (numSamples+1e-9));
            c.shortTermRMS= stCoef*c.shortTermRMS + (1.f - stCoef)*blkRms;
            c.longTermRMS = ltCoef*c.longTermRMS  + (1.f - ltCoef)*blkRms;
            continue;
        }

        // If bypass or automix off => pass-through but measure RMS
        if (c.bypass || !c.automix)
        {
            double sumSq=0.0;
            for (int i=0; i<numSamples; ++i)
                sumSq += blockData[ch][i] * blockData[ch][i];
            float blkRms = (float)std::sqrt(sumSq / (numSamples+1e-9));
            c.shortTermRMS= stCoef*c.shortTermRMS + (1.f - stCoef)*blkRms;
            c.longTermRMS = ltCoef*c.longTermRMS  + (1.f - ltCoef)*blkRms;

            float stDb = linToDb(blkRms) + c.sensDb + c.faderDb;
            c.finalGain = dbToLin(stDb) * masterGain.load();
            c.gateActive= false;
            continue;
        }

        // Normal automix channel
        double sumSq=0.0;
        for (int i=0; i<numSamples; ++i)
            sumSq += blockData[ch][i] * blockData[ch][i];
        float blkRms = (float)std::sqrt(sumSq / (numSamples+1e-9));

        c.shortTermRMS= stCoef*c.shortTermRMS + (1.f - stCoef)*blkRms;
        c.longTermRMS = ltCoef*c.longTermRMS  + (1.f - ltCoef)*blkRms;

        float stDb = linToDb(c.shortTermRMS) + c.sensDb;

        bool wasActive = c.gateActive;
        bool wantOpen  = false;

        if (wasActive)
        {
            if (stDb > gateOff)
                wantOpen = true;
        }
        else
        {
            if (stDb > gateOn)
                wantOpen = true;
        }

        float target = wantOpen ? 1.f : 0.f;
        float coeff  = wantOpen ? attCoeff : relCoeff;
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

    // "Last mic on" logic
    if (!anyActive && lastMicOn.load())
    {
        channels[lastActiveChannel].gateActive = true;
        channels[lastActiveChannel].gateEnv    = 1.f;
    }
    else if (anyActive)
    {
        lastActiveChannel = loudestCh;
    }

    // 5) Gain share
    double sumActive = 0.0;
    for (int ch=0; ch<numCh; ++ch)
    {
        auto& c = channels[ch];
        if (c.mute || c.bypass || !c.automix)
            continue;
        if (c.gateActive)
        {
            float stDb = linToDb(c.shortTermRMS + 1e-9f);
            sumActive += dbToLin(stDb);
        }
    }

    for (int ch=0; ch<numCh; ++ch)
    {
        auto& c = channels[ch];
        if (c.mute || c.bypass || !c.automix)
            continue;

        if (!c.gateActive)
        {
            c.finalGain = dbToLin(closeDb) * c.gateEnv;
        }
        else
        {
            float stDb = linToDb(c.shortTermRMS + 1e-9f);
            float lin  = dbToLin(stDb);
            if (sumActive < 1e-9)
                c.finalGain = 1.f / (float) numCh;
            else
                c.finalGain = lin / (float) sumActive;
        }
        // Apply user fader + master
        c.finalGain *= dbToLin(c.faderDb);
        c.finalGain *= masterGain.load();
    }

    // link leveler => clamp
    if (linkLeveler.load())
    {
        float maxLin = dbToLin(levelerRangeDb.load());
        for (auto& c : channels)
        {
            if (c.finalGain > maxLin)
                c.finalGain = maxLin;
        }
    }

    // 6) Write final gain to output
    for (int ch=0; ch<numCh; ++ch)
    {
        float g = channels[ch].finalGain;
        for (int i=0; i<numSamples; ++i)
            mainData[ch][i] *= g;
    }
}

float MyDuganAutomixer::computeAdaptiveThreshold(float baseThresh, float sideDb)
{
    // Example offset
    float infl = sidechainInfluence.load();
    float offset = infl * (sideDb + 40.f) / 40.f; // Arbitrary example formula
    return baseThresh + offset;
}

// Channel set methods
void MyDuganAutomixer::setChannelMute(int ch, bool b)
{
    if (ch >= 0 && ch < (int)channels.size())
        channels[ch].mute = b;
}
void MyDuganAutomixer::setChannelBypass(int ch, bool b)
{
    if (ch >= 0 && ch < (int)channels.size())
        channels[ch].bypass = b;
}
void MyDuganAutomixer::setChannelAutomixOn(int ch, bool b)
{
    if (ch >= 0 && ch < (int)channels.size())
        channels[ch].automix = b;
}
void MyDuganAutomixer::setChannelSensDb(int ch, float dB)
{
    if (ch >= 0 && ch < (int)channels.size())
        channels[ch].sensDb = dB;
}
void MyDuganAutomixer::setChannelFaderDb(int ch, float dB)
{
    if (ch >= 0 && ch < (int)channels.size())
        channels[ch].faderDb = dB;
}

float MyDuganAutomixer::getChannelShortTermRMS(int ch) const
{
    if (ch < 0 || ch >= (int)channels.size()) return 0.f;
    return channels[ch].shortTermRMS;
}
float MyDuganAutomixer::getChannelAutoGainDb(int ch) const
{
    if (ch < 0 || ch >= (int)channels.size()) return 0.f;
    return linToDb(channels[ch].finalGain);
}
