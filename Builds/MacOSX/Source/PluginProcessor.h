#pragma once

#include <JuceHeader.h>
#include "EnhancedDuganAGC.h"

/**
    MyDuganPluginAudioProcessor:
    - A bus plugin that accepts 4 mono inputs.
    - Implements a Dugan-inspired gain-sharing algorithm (via EnhancedDuganAGC) to mix the 4 channels.
*/
class MyDuganPluginAudioProcessor : public juce::AudioProcessor
{
public:
    MyDuganPluginAudioProcessor();
    ~MyDuganPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override               { return "MyDuganPlugin"; }
    bool acceptsMidi() const override                         { return false; }
    bool producesMidi() const override                        { return false; }
    double getTailLengthSeconds() const override              { return 0.0; }

    //==============================================================================
    int getNumPrograms() override                             { return 1; }
    int getCurrentProgram() override                          { return 0; }
    void setCurrentProgram (int index) override               {}
    const juce::String getProgramName (int index) override    { return {}; }
    void changeProgramName (int index, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Our parameter tree
    juce::AudioProcessorValueTreeState parameters;

    // Our enhanced automixer (now configured for 4 channels)
    EnhancedDuganAGC agc;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyDuganPluginAudioProcessor)
};
