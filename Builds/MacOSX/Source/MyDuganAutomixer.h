#pragma once

#include <atomic>
#include <vector>

/**
    MyDuganAutomixer:
     - Dugan‐style gain sharing for multiple channels
     - Attack/release gating
     - Optional lookahead
     - Optional sidechain for adaptive threshold
     - "Last mic on" logic
*/
class MyDuganAutomixer
{
public:
    MyDuganAutomixer() = default;
    ~MyDuganAutomixer() = default;

    // Prepare for mainChannels and sideChainChannels
    void prepare (double sampleRate, int blockSize, int mainChannels, int sideChainChannels);

    // Process audio in-place
    void processBlock(float** mainData, int mainCh, int numSamples,
                      float** sideData, int sideCh, int sideSamples);

    //====================================================================
    // Setters
    void setMasterGain (float mg)  { masterGain.store(mg); }

    void setGateThreshold  (float dB) { gateThreshold.store(dB); }
    void setGateHysteresis (float dB) { gateHysteresis.store(dB); }
    void setGateCloseDb    (float dB) { gateCloseDb.store(dB); }
    void setGateAttackMs   (float ms) { gateAttackMs.store(ms); }
    void setGateReleaseMs  (float ms) { gateReleaseMs.store(ms); }
    void setLastMicOn      (bool b)   { lastMicOn.store(b); }

    void setShortTermMs    (float ms) { shortTermMs.store(ms); }
    void setLongTermMs     (float ms) { longTermMs.store(ms); }

    void setLookaheadMs    (float ms);

    void setLinkLeveler    (bool b)   { linkLeveler.store(b); }
    void setLevelerRangeDb (float dB) { levelerRangeDb.store(dB); }

    void setUseAdaptiveThreshold (bool b) { useAdaptiveThreshold.store(b); }
    void setSidechainInfluence   (float f){ sidechainInfluence.store(f); }

    // Per-channel
    void setChannelMute      (int ch, bool b);
    void setChannelBypass    (int ch, bool b);
    void setChannelAutomixOn (int ch, bool b);
    void setChannelSensDb    (int ch, float dB);
    void setChannelFaderDb   (int ch, float dB);

    //====================================================================
    // For UI
    float getChannelShortTermRMS (int ch) const;
    float getChannelAutoGainDb   (int ch) const;

private:
    // Helper conversions
    static float dbToLin(float dB);
    static float linToDb(float lin);

    float computeAdaptiveThreshold(float baseThresh, float sideDb);

    struct ChannelInfo
    {
        bool  mute      = false;
        bool  bypass    = false;
        bool  automix   = true;
        float sensDb    = 0.f;
        float faderDb   = 0.f;

        float shortTermRMS = 0.f;
        float longTermRMS  = 0.f;
        float gateEnv      = 0.f; // Attack/Release envelope
        bool  gateActive   = false;
        float finalGain    = 1.f;
    };

    // Channels
    std::vector<ChannelInfo> channels;

    // Audio settings
    double sr        = 44100.0;
    int    blockSize = 512;
    int    numCh     = 0;
    int    sideCh    = 0;

    // Parameters
    std::atomic<float> masterGain      {1.f};

    std::atomic<float> gateThreshold   {-40.f};
    std::atomic<float> gateHysteresis  { 3.f};
    std::atomic<float> gateCloseDb     {-30.f};
    std::atomic<float> gateAttackMs    {10.f};
    std::atomic<float> gateReleaseMs   {200.f};
    std::atomic<bool>  lastMicOn       {true};

    std::atomic<float> shortTermMs     {20.f};
    std::atomic<float> longTermMs      {500.f};

    // Lookahead
    std::atomic<float> lookaheadMs     {0.f};
    std::vector<float> lookaheadBuffer;
    int lookaheadBufferSize = 0;
    int writePos            = 0;

    std::atomic<bool>  linkLeveler     {false};
    std::atomic<float> levelerRangeDb  {12.f};

    // Adaptive threshold
    std::atomic<bool>  useAdaptiveThreshold {false};
    std::atomic<float> sidechainInfluence   {0.f};

    int lastActiveChannel = 0;
};
