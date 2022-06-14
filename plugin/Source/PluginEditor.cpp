#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace gin;

//==============================================================================
OrganAudioProcessorEditor::OrganAudioProcessorEditor (OrganAudioProcessor& p)
    : ProcessorEditor (p), proc (p)
{
    setGridSize (10, 3);
}

OrganAudioProcessorEditor::~OrganAudioProcessorEditor()
{
}

//==============================================================================
void OrganAudioProcessorEditor::paint (juce::Graphics& g)
{
    ProcessorEditor::paint (g);
}

void OrganAudioProcessorEditor::resized()
{
    ProcessorEditor::resized ();
}
