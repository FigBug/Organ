#pragma once

#include <JuceHeader.h>

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

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    void updateParams (int blockSize);
    void setupModMatrix();

    gin::BandLimitedLookupTables bandLimitedLookupTables;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganAudioProcessor)
};
