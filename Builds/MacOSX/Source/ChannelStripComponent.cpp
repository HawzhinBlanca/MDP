#include "ChannelStripComponent.h"

ChannelStripComponent::ChannelStripComponent(juce::AudioProcessorValueTreeState& vts, const juce::String& channelID)
    : parameters(vts), channelID(channelID)
{
    // Setup the fader
    addAndMakeVisible(faderSlider);
    faderSlider.setSliderStyle(juce::Slider::LinearVertical);
    faderSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    // Attach fader to parameter
    faderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        parameters, channelID + "Fader", faderSlider);
    
    // Setup Mute button
    addAndMakeVisible(muteButton);
    muteButton.setButtonText("M");
    muteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        parameters, channelID + "Mute", muteButton);
    
    // Setup Solo button
    addAndMakeVisible(soloButton);
    soloButton.setButtonText("S");
    soloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        parameters, channelID + "Solo", soloButton);
}

ChannelStripComponent::~ChannelStripComponent() {}

void ChannelStripComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Fill background with a gradient
    juce::ColourGradient bgGrad(juce::Colours::black, bounds.getX(), bounds.getY(),
                                juce::Colours::darkgrey, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 6.0f);
    
    // Neon border
    g.setColour(juce::Colours::limegreen);
    g.drawRoundedRectangle(bounds, 6.0f, 2.0f);
    
    // Level meter on the left side
    int meterWidth = 10;
    int meterHeight = (int) (rmsLevel * bounds.getHeight());
    juce::Rectangle<float> meterArea (bounds.getX() + 4,
                                      bounds.getBottom() - meterHeight - 4,
                                      (float) meterWidth, (float) meterHeight);
    juce::ColourGradient meterGrad (juce::Colours::limegreen, meterArea.getX(), meterArea.getY(),
                                    juce::Colours::green, meterArea.getRight(), meterArea.getBottom(), false);
    g.setGradientFill(meterGrad);
    g.fillRect(meterArea);
    
    // LED top-right corner
    int ledSize = 12;
    juce::Rectangle<float> ledRect (bounds.getRight() - (ledSize + 6), bounds.getY() + 6, (float) ledSize, (float) ledSize);
    g.setColour(voiceActive ? juce::Colours::red : juce::Colours::grey);
    g.fillEllipse(ledRect);
    
    // Channel label at top center
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    auto labelArea = bounds.withTop(4).withHeight(20).toNearestInt();
    g.drawText(channelID, labelArea, juce::Justification::centred);
}

void ChannelStripComponent::resized()
{
    auto area = getLocalBounds();
    
    // Reserve some space for the fader on the right side
    auto faderArea = area.removeFromRight(60).reduced(6);
    faderSlider.setBounds(faderArea);
    
    // At the bottom, place Mute and Solo
    auto buttonArea = area.removeFromBottom(40).reduced(4);
    auto halfWidth = buttonArea.getWidth() / 2;
    muteButton.setBounds(buttonArea.removeFromLeft(halfWidth).reduced(4));
    soloButton.setBounds(buttonArea.reduced(4));
}

void ChannelStripComponent::setRMSLevel(float level)
{
    rmsLevel = level;
    repaint();
}

void ChannelStripComponent::setVoiceActive(bool active)
{
    voiceActive = active;
    repaint();
}
