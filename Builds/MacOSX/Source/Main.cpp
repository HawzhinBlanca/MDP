// Main.cpp
#include "PluginProcessor.h"
#include <JuceHeader.h>
#include "ChannelStripComponent.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyDuganPluginAudioProcessor();
}
