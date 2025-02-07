#pragma once

/**
    A stub for a machine-learning speech detector.
    You can integrate a TensorFlow Lite model or an external library.
    Purely virtual interface here.
*/
class MLSpeechDetector
{
public:
    MLSpeechDetector() {}
    virtual ~MLSpeechDetector() {}

    // Provide raw audio data for one channel; return true if likely speech
    virtual bool detectSpeech(const float* samples, int numSamples, int channelIndex) = 0;
};
