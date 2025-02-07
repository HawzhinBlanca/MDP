// EnhancedDuganAGC.h
#pragma once

#include <atomic>
#include <vector>
#include <memory>
#include "LockFreeFifo.h"

// Forward declaration for a simple SpeechResult struct.
struct SpeechResult
{
    bool isActive = false;
    float timeStamp = 0.f;
};

/**
    EnhancedDuganAGC:
    - Integrates Dugan-style gain sharing with additional machine learning
      voice activity detection (VAD) and optional cross-talk suppression.
    - Includes a lookahead buffer and supports real-time adaptive thresholding.
*/
class EnhancedDuganAGC
{
public:
    EnhancedDuganAGC() = default;
    ~EnhancedDuganAGC() = default;

    // Prepare AGC for a given sample rate, block size, number of channels, and optional sidechain count.
    void prepare(double sampleRate, int blockSize, int mainChannels, int sideChainCount);

    // Process audio in real time:
    void processBlock(float** mainData, int mainCh, int numSamples,
                      float** sideData, int sideCh, int sideSamples);

    // Parameter setters:
    void setMasterGain(float g)          { masterGain.store(g); }
    void setGateThreshold(float dB)      { gateThreshold.store(dB); }
    void setGateHysteresis(float dB)     { gateHysteresis.store(dB); }
    void setGateCloseDb(float dB)        { gateCloseDb.store(dB); }
    void setGateAttackMs(float ms)       { gateAttackMs.store(ms); }
    void setGateReleaseMs(float ms)      { gateReleaseMs.store(ms); }
    void setLastMicOn(bool b)            { lastMicOn.store(b); }
    void setLookaheadMs(float ms);
    void setShortTermMs(float ms)        { shortTermMs.store(ms); }
    void setLongTermMs(float ms)         { longTermMs.store(ms); }
    void setLinkLeveler(bool b)          { linkLeveler.store(b); }
    void setLevelerRangeDb(float dB)     { levelerRangeDb.store(dB); }
    void setUseAdaptiveThreshold(bool b) { useAdaptiveThreshold.store(b); }
    void setSidechainInfluence(float f)  { sidechainInfluence.store(f); }

    // ML/VAD and per-channel settings:
    void setUseMLSpeechDetection(bool b) { useMLSpeechDetection.store(b); }
    void setChannelMute(int ch, bool b);
    void setChannelBypass(int ch, bool b);
    void setChannelAutomixOn(int ch, bool b);
    void setChannelSensDb(int ch, float dB);
    void setChannelFaderDb(int ch, float dB);

    // For UI meters:
    float getChannelShortTermRMS(int ch) const;
    float getChannelAutoGainDb(int ch) const;

private:
    struct ChannelInfo
    {
        bool mute      = false;
        bool bypass    = false;
        bool automix   = true;
        float sensDb   = 0.f;
        float faderDb  = 0.f;

        float shortTermRMS = 0.f;
        float longTermRMS  = 0.f;
        float gateEnv      = 0.f;
        bool gateActive    = false;
        float finalGain    = 1.f;
    };

    // Core processing functions:
    void processBlockInternal(float** audioData, int nChannels, int nSamples,
                              float** sideData, int sideCh, int sideSamples);
    float computeAdaptiveThreshold(float baseThresh, float sidechainDb);
    static float dbToLinear(float dB);
    static float linearToDb(float lin);

    // For ML/VAD: a stub to update channel speech states from a lock-free FIFO.
    void updateMLSpeechStates();

    // Audio settings:
    double sr = 44100.0;
    int blockSize = 512;
    int numCh = 0;
    int sideCh = 0;

    std::vector<ChannelInfo> channels;

    // Lookahead buffer:
    std::atomic<float> lookaheadMs {0.f};
    std::vector<float> lookaheadBuffer;
    int lookaheadBufferSize = 0;
    int writePos = 0;

    // AGC parameters:
    std::atomic<float> masterGain {1.f};
    std::atomic<float> gateThreshold {-40.f};
    std::atomic<float> gateHysteresis {3.f};
    std::atomic<float> gateCloseDb {-30.f};
    std::atomic<float> gateAttackMs {10.f};
    std::atomic<float> gateReleaseMs {200.f};
    std::atomic<bool>  lastMicOn {true};

    std::atomic<float> shortTermMs {20.f};
    std::atomic<float> longTermMs  {500.f};

    std::atomic<bool> linkLeveler {false};
    std::atomic<float> levelerRangeDb {12.f};

    std::atomic<bool> useAdaptiveThreshold {false};
    std::atomic<float> sidechainInfluence {0.f};

    // ML/VAD integration:
    std::atomic<bool> useMLSpeechDetection {false};
    // In a real implementation, speechResultsFifo would be a lock-free FIFO containing SpeechResult structs.
    // For this demo, we'll assume it's a pointer that can be set externally.
    std::shared_ptr<LockFreeFifo<SpeechResult>> speechResultsFifo;
    bool mlSpeechActiveForChannel[32] = { false };

    int lastActiveChannel = 0;
};
