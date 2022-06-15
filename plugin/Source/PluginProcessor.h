#pragma once

#include <JuceHeader.h>
#include "Organ.h"

//==============================================================================
class OrganAudioProcessor : public gin::Processor
{
public:
    //==============================================================================
    OrganAudioProcessor();
    ~OrganAudioProcessor() override;

    void stateUpdated() override;
    void updateState() override;

    //==============================================================================
    void reset() override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    std::unique_ptr<Organ> organ;

    gin::Parameter::Ptr upperDrawBars[9];
    gin::Parameter::Ptr lowerDrawBars[9];
    gin::Parameter::Ptr pedalDrawBars[9];

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganAudioProcessor)
};
