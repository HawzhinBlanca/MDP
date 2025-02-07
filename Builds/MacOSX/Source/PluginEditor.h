#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ChannelStripComponent.h"
#include "MyCustomLookAndFeel.h"

// Example AutomixerPanel
class AutomixerPanel : public juce::Component
{
public:
    AutomixerPanel(juce::AudioProcessorValueTreeState& vts)
      : parameters (vts)
    {
        // Example: an on/off toggle
        addAndMakeVisible(enableButton);
        enableButton.setButtonText("Enable Automixer");
        enableButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            parameters, "enableAutomixer", enableButton);
        
        // Example: a rotary slider for "mixingRate"
        addAndMakeVisible(mixingRateSlider);
        mixingRateSlider.setSliderStyle(juce::Slider::Rotary);
        mixingRateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        mixingRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "mixingRate", mixingRateSlider);
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(8);
        enableButton.setBounds(area.removeFromTop(30));
        mixingRateSlider.setBounds(area.reduced(8));
    }

private:
    juce::AudioProcessorValueTreeState& parameters;
    juce::ToggleButton enableButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableButtonAttachment;
    
    juce::Slider mixingRateSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixingRateAttachment;
};

// Example NoiseGatePanel
class NoiseGatePanel : public juce::Component
{
public:
    NoiseGatePanel(juce::AudioProcessorValueTreeState& vts)
      : parameters (vts)
    {
        // Example: threshold, attack, release
        for (auto* slider : { &thresholdSlider, &attackSlider, &releaseSlider })
        {
            addAndMakeVisible(slider);
            slider->setSliderStyle(juce::Slider::LinearHorizontal);
            slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);
        }
        
        thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "gateThreshold", thresholdSlider);
        attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "gateAttack", attackSlider);
        releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "gateRelease", releaseSlider);
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(8);
        thresholdSlider.setBounds(area.removeFromTop(40));
        attackSlider.setBounds(area.removeFromTop(40));
        releaseSlider.setBounds(area.removeFromTop(40));
    }

private:
    juce::AudioProcessorValueTreeState& parameters;
    
    juce::Slider thresholdSlider, attackSlider, releaseSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment, attackAttachment, releaseAttachment;
};

// Example AdvancedPanel
class AdvancedPanel : public juce::Component
{
public:
    AdvancedPanel(juce::AudioProcessorValueTreeState& vts)
      : parameters(vts)
    {
        // A combo box for "preset"
        addAndMakeVisible(presetBox);
        presetBox.addItem("Default", 1);
        presetBox.addItem("Live", 2);
        presetBox.addItem("Podcast", 3);
        presetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            parameters, "preset", presetBox);
        
        // A slider for lookahead
        addAndMakeVisible(lookaheadSlider);
        lookaheadSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        lookaheadSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);
        lookaheadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "lookahead", lookaheadSlider);
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(8);
        presetBox.setBounds(area.removeFromTop(30));
        lookaheadSlider.setBounds(area.removeFromTop(40));
    }

private:
    juce::AudioProcessorValueTreeState& parameters;
    juce::ComboBox presetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> presetAttachment;
    
    juce::Slider lookaheadSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lookaheadAttachment;
};

class MyDuganPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    MyDuganPluginAudioProcessorEditor (MyDuganPluginAudioProcessor&);
    ~MyDuganPluginAudioProcessorEditor() override;
    
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    
    MyDuganPluginAudioProcessor& audioProcessor;
    
    // Custom LookAndFeel instance
    MyCustomLookAndFeel customLF;
    
    // Example: 4 channel strips
    juce::OwnedArray<ChannelStripComponent> channelStrips;
    
    // Panels
    AutomixerPanel automixerPanel;
    NoiseGatePanel noiseGatePanel;
    AdvancedPanel advancedPanel;
    
    // Master gain slider
    juce::Slider masterGainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyDuganPluginAudioProcessorEditor)
};
