#pragma once
#include <JuceHeader.h>

class SettingsPanel : public juce::Component
{
public:
    SettingsPanel (const juce::String& name, juce::Colour bgColour)
        : panelName(name), backgroundColour(bgColour)
    {
        setName(name);
    }
    
    void paint (juce::Graphics& g) override
    {
        // Fill the background with the specified color.
        g.fillAll(backgroundColour);
        
        // Draw the panel name in white in the centre.
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText(panelName, getLocalBounds(), juce::Justification::centred, true);
    }
    
private:
    juce::String panelName;
    juce::Colour backgroundColour;
};
