#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ChannelStripComponent.h"

static constexpr int kMainChannels = 4; // 4 mono tracks

// Constructor
MyDuganPluginAudioProcessor::MyDuganPluginAudioProcessor()
  : AudioProcessor (BusesProperties()
      // One input bus: a discrete set of 4 mono channels.
      .withInput("Inputs", juce::AudioChannelSet::discreteChannels(kMainChannels), true)
      // One output bus: stereo output.
      .withOutput("MainOut", juce::AudioChannelSet::stereo(), true)
    ),
    parameters(*this, nullptr, "PARAMS",
    {
        // [Parameter definitions go here...]
        // For example:
        std::make_unique<juce::AudioParameterFloat>("gateThreshold", "Gate Threshold (dB)", -60.f, 0.f, -40.f),
        // ... (all the rest of your parameter definitions) ...
    })
{
    // Constructor implementation (if any)
}

// Destructor (must be defined, even if empty)
MyDuganPluginAudioProcessor::~MyDuganPluginAudioProcessor()
{
    // Empty destructor is fine.
}

// prepareToPlay
void MyDuganPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    agc.prepare(sampleRate, samplesPerBlock, kMainChannels, 0); // 0 sidechain channels for now
}

// processBlock
void MyDuganPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    
    int nSamples = buffer.getNumSamples();
    auto* channelData = buffer.getArrayOfWritePointers();

    // Build an array of pointers for the 4 channels:
    std::vector<float*> channels;
    for (int ch = 0; ch < kMainChannels; ++ch)
    {
        channels.push_back(channelData[ch]);
    }

    // Process the block using our automixer:
    agc.processBlock(channels.data(), kMainChannels, nSamples, nullptr, 0, 0);

    // Mix the channels into a stereo output:
    auto outBus = getBusBuffer(buffer, false, 0);
    float* outL = outBus.getWritePointer(0);
    float* outR = outBus.getWritePointer(1);

    for (int i = 0; i < nSamples; ++i)
    {
        float mix = 0.f;
        for (int ch = 0; ch < kMainChannels; ++ch)
            mix += channels[ch][i];
        outL[i] = mix;
        outR[i] = mix;
    }
}

// createEditor: Return a pointer to your editor.
juce::AudioProcessorEditor* MyDuganPluginAudioProcessor::createEditor()
{
    return new MyDuganPluginAudioProcessorEditor(*this);
}

// releaseResources: Called when playback stops. (Empty is fine.)
void MyDuganPluginAudioProcessor::releaseResources()
{
    // If you have any allocated resources to free, do it here.
}

// getStateInformation: Save the plugin’s state.
void MyDuganPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, true);
    parameters.state.writeToStream(stream);
}

// setStateInformation: Restore the plugin’s state.
void MyDuganPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
        parameters.replaceState(tree);
}

// isBusesLayoutSupported: Check if the given layout is acceptable.
bool MyDuganPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Check the input bus: it must be a discrete set of 4 channels.
    if (layouts.inputBuses.size() > 0)
    {
        auto inputSet = layouts.getChannelSet(true, 0);
        if (inputSet != juce::AudioChannelSet::discreteChannels(kMainChannels))
            return false;
    }
    
    // Check the output bus: it must be stereo.
    if (layouts.outputBuses.size() > 0)
    {
        auto outputSet = layouts.getChannelSet(false, 0);
        if (outputSet != juce::AudioChannelSet::stereo())
            return false;
    }
    return true;
}
