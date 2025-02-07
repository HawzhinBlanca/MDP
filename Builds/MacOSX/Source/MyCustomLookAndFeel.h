#pragma once
#include <JuceHeader.h>

class MyCustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MyCustomLookAndFeel()
    {
        // Example: set a few colours globally (if desired).
        // For instance, you can control default text box background:
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::black);
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::darkgrey);
        
        // Similarly for combos, if you use them:
        setColour(juce::ComboBox::backgroundColourId, juce::Colours::black);
        setColour(juce::ComboBox::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId, juce::Colours::darkgrey);
    }
    
    //------------------------------------------------------------------------
    // Custom Rotary Slider
    //------------------------------------------------------------------------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, const float rotaryStartAngle,
                           const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius   = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX  = (float) x + (float) width  * 0.5f;
        auto centreY  = (float) y + (float) height * 0.5f;
        auto rx       = centreX - radius;
        auto ry       = centreY - radius;
        auto diameter = radius * 2.0f;
        
        // Background ellipse
        juce::ColourGradient bgGrad (juce::Colours::black, centreX, centreY - radius,
                                     juce::Colours::darkgrey, centreX, centreY + radius, true);
        g.setGradientFill(bgGrad);
        g.fillEllipse(rx, ry, diameter, diameter);
        
        // Outline
        g.setColour(juce::Colours::limegreen);
        g.drawEllipse(rx, ry, diameter, diameter, 2.0f);
        
        // Draw the rotary pointer.
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto pointerLength = radius * 0.8f;
        auto pointerThickness = 4.0f;
        
        juce::Line<float> line (centreX, centreY,
                                centreX + pointerLength * std::cos(angle),
                                centreY + pointerLength * std::sin(angle));
        
        g.setColour(juce::Colours::limegreen);
        g.drawLine(line, pointerThickness);
    }
    
    //------------------------------------------------------------------------
    // Custom Linear Slider
    //------------------------------------------------------------------------
    void drawLinearSlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            // Background gradient
            juce::ColourGradient bgGrad (juce::Colours::darkgrey, (float) x, (float) y,
                                         juce::Colours::black, (float) x, (float) (y + height), false);
            g.setGradientFill(bgGrad);
            g.fillRect(x, y, width, height);
            
            // Neon track in the center
            int trackWidth = juce::jmax(2, width / 4); // ensure track is at least 2 px wide
            int trackX = x + (width - trackWidth) / 2;
            g.setColour(juce::Colours::limegreen);
            g.fillRect(trackX, y, trackWidth, height);
            
            // White “thumb” rectangle
            int thumbHeight = 20;
            int thumbY = (int) (sliderPos - (thumbHeight / 2));
            thumbY = juce::jlimit(y, y + height - thumbHeight, thumbY);
            g.setColour(juce::Colours::white);
            g.fillRect(x, thumbY, width, thumbHeight);
            
            // Outline around the thumb
            g.setColour(juce::Colours::limegreen);
            g.drawRect(x, thumbY, width, thumbHeight, 2);
        }
        else
        {
            // Let the default look handle other styles (LinearHorizontal, etc.)
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                              minSliderPos, maxSliderPos, style, slider);
        }
    }
    
    //------------------------------------------------------------------------
    // Custom Toggle Button
    //------------------------------------------------------------------------
    void drawToggleButton (juce::Graphics& g,
                           juce::ToggleButton& button,
                           bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        
        // Base colour depends on toggle state
        auto baseColor = button.getToggleState() ? juce::Colours::limegreen : juce::Colours::darkgrey;
        g.setColour(baseColor);
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // White outline
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        
        // Text
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred, true);
    }
};
