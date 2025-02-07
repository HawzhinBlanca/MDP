#pragma once
#include <JuceHeader.h>

/** A single channel strip with a vertical fader, Mute, and Solo toggle buttons. */
class ChannelStripComponent : public juce::Component
{
public:
    ChannelStripComponent(juce::AudioProcessorValueTreeState& vts, const juce::String& channelID);
    ~ChannelStripComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setRMSLevel(float level);
    void setVoiceActive(bool active);
    
private:
    juce::AudioProcessorValueTreeState& parameters;
    juce::String channelID;
    
    // The channel fader
    juce::Slider faderSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> faderAttachment;
    
    // Mute and Solo toggles
    juce::ToggleButton muteButton;
    juce::ToggleButton soloButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttachment;
    
    float rmsLevel = 0.0f;
    bool voiceActive = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStripComponent)
};
