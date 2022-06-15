#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class OrganAudioProcessorEditor : public gin::ProcessorEditor
{
public:
    OrganAudioProcessorEditor (OrganAudioProcessor&);
    ~OrganAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    gin::Layout layout {*this};
    
    OrganAudioProcessor& proc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganAudioProcessorEditor)
};
