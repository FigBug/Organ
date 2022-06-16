#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Drawbar.h"

//==============================================================================
OrganAudioProcessorEditor::OrganAudioProcessorEditor (OrganAudioProcessor& p)
    : ProcessorEditor (p), proc (p)
{
    upperKeyboard.setName ("upperKeys");
    upperKeyboard.setMidiChannel (1);
    upperKeyboard.setMidiChannelsToDisplay (1 << 0);
    upperKeyboard.setKeyWidth (20);
    upperKeyboard.setAvailableRange (36, 96);
    upperKeyboard.setScrollButtonsVisible (false);
    addAndMakeVisible (upperKeyboard);

    lowerKeyboard.setName ("lowerKeys");
    lowerKeyboard.setMidiChannel (2);
    lowerKeyboard.setMidiChannelsToDisplay (1 << 1);
    lowerKeyboard.setKeyWidth (20);
    lowerKeyboard.setAvailableRange (36, 96);
    lowerKeyboard.setScrollButtonsVisible (false);
    addAndMakeVisible (lowerKeyboard);

    pedalKeyboard.setName ("pedalKeys");
    pedalKeyboard.setMidiChannel (3);
    pedalKeyboard.setMidiChannelsToDisplay (1 << 2);
    pedalKeyboard.setKeyWidth (20);
    pedalKeyboard.setAvailableRange (24, 49);
    pedalKeyboard.setScrollButtonsVisible (false);
    addAndMakeVisible (pedalKeyboard);
    
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
