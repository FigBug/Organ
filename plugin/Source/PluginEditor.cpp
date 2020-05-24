#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace gin;

//==============================================================================
OrganAudioProcessorEditor::OrganAudioProcessorEditor (OrganAudioProcessor& p)
    : ProcessorEditor (p, 50, 50 + 15), proc (p)
{
}

OrganAudioProcessorEditor::~OrganAudioProcessorEditor()
{
}

//==============================================================================
void OrganAudioProcessorEditor::paint (Graphics& g)
{
    ProcessorEditor::paint (g);
}

void OrganAudioProcessorEditor::resized()
{
    ProcessorEditor::resized ();
}
