#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Drawbar.h"

//==============================================================================
OrganAudioProcessorEditor::OrganAudioProcessorEditor (OrganAudioProcessor& p)
    : ProcessorEditor (p), proc (p)
{
    for (auto pp : p.getPluginParameters())
    {
        auto pc = new Drawbar (pp);

        addAndMakeVisible (pc);
        controls.add (pc);
    }

    setGridSize (15, 5);

    layout.setLayout ("ui.json", juce::File (__FILE__).getSiblingFile ("ui.json"));
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

    int i = 0;
    for (auto pp : proc.getPluginParameters())
    {
        componentForParam (*pp)->setBounds (getGridArea (i % 9, i / 9));
        i++;
    }
}
