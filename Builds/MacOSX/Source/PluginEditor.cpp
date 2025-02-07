#include "PluginEditor.h"
#include "PluginProcessor.h"

MyDuganPluginAudioProcessorEditor::MyDuganPluginAudioProcessorEditor (MyDuganPluginAudioProcessor& p)
    : AudioProcessorEditor(p),
      audioProcessor(p),
      automixerPanel(audioProcessor.parameters),
      noiseGatePanel(audioProcessor.parameters),
      advancedPanel(audioProcessor.parameters)
{
    setSize(1000, 600);
    
    // Apply our custom LookAndFeel to the entire editor
    setLookAndFeel(&customLF);
    
    // Create channel strips
    for (int i = 0; i < 4; ++i)
    {
        auto channelName = "Ch " + juce::String(i + 1);
        auto* strip = new ChannelStripComponent(audioProcessor.parameters, channelName);
        channelStrips.add(strip);
        addAndMakeVisible(strip);
    }
    
    // Master gain slider
    addAndMakeVisible(masterGainSlider);
    masterGainSlider.setSliderStyle(juce::Slider::Rotary);
    masterGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    masterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "masterGain", masterGainSlider);
    
    // Add the panels
    addAndMakeVisible(automixerPanel);
    addAndMakeVisible(noiseGatePanel);
    addAndMakeVisible(advancedPanel);
    
    // Start a timer to update the channel strip levels
    startTimerHz(15);
}

MyDuganPluginAudioProcessorEditor::~MyDuganPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void MyDuganPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText("MyDuganPlugin - 4-Channel Automixer", getLocalBounds().reduced(10),
                     juce::Justification::horizontallyCentred | juce::Justification::top, 1);
}

void MyDuganPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Top area: master slider + plugin title
    auto topArea = area.removeFromTop(150);
    masterGainSlider.setBounds(topArea.removeFromRight(150).reduced(10));
    
    // 3 panels in the remaining topArea
    auto panelWidth = topArea.getWidth() / 3;
    automixerPanel.setBounds(topArea.removeFromLeft(panelWidth).reduced(10));
    noiseGatePanel.setBounds(topArea.removeFromLeft(panelWidth).reduced(10));
    advancedPanel.setBounds(topArea.reduced(10));
    
    // Bottom area for channel strips
    auto channelArea = area;
    int numStrips = channelStrips.size();
    int stripWidth = channelArea.getWidth() / numStrips;
    for (int i = 0; i < numStrips; ++i)
    {
        channelStrips[i]->setBounds(channelArea.removeFromLeft(stripWidth).reduced(10));
    }
}

void MyDuganPluginAudioProcessorEditor::timerCallback()
{
    // Example: update each strip's RMS and voice active
    for (int i = 0; i < channelStrips.size(); ++i)
    {
        float rmsVal = audioProcessor.agc.getChannelShortTermRMS(i);
        channelStrips[i]->setRMSLevel(rmsVal);

        bool active = audioProcessor.agc.getChannelAutoGainDb(i) > -20.f;
        channelStrips[i]->setVoiceActive(active);
    }
    
    repaint();
}
