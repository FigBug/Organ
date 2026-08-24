#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "BacksidePanel.h"

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
    gin::LayoutSupport layout {*this};

    OrganAudioProcessor& proc;

    juce::MidiKeyboardComponent upperKeyboard { proc.upperState, juce::MidiKeyboardComponent::horizontalKeyboard };
    juce::MidiKeyboardComponent lowerKeyboard { proc.lowerState, juce::MidiKeyboardComponent::horizontalKeyboard };
    juce::MidiKeyboardComponent pedalKeyboard { proc.pedalState, juce::MidiKeyboardComponent::horizontalKeyboard };

    gin::SVGButton gearButton;
    juce::Viewport backsideViewport;
    BacksidePanel backsidePanel { proc };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganAudioProcessorEditor)
};
