#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Drawbar.h"

//==============================================================================
OrganAudioProcessorEditor::OrganAudioProcessorEditor (OrganAudioProcessor& p_)
    : ProcessorEditor (p_), proc (p_)
{
    setName ("main");
    
    upperKeyboard.setName ("upperKeys");
    upperKeyboard.setOpaque (false);
    upperKeyboard.setMidiChannel (1);
    upperKeyboard.setMidiChannelsToDisplay (1 << 0);
    upperKeyboard.setKeyWidth (20);
    upperKeyboard.setAvailableRange (36, 96);
    upperKeyboard.setScrollButtonsVisible (false);
    addAndMakeVisible (upperKeyboard);

    lowerKeyboard.setName ("lowerKeys");
    lowerKeyboard.setOpaque (false);
    lowerKeyboard.setMidiChannel (2);
    lowerKeyboard.setMidiChannelsToDisplay (1 << 1);
    lowerKeyboard.setKeyWidth (20);
    lowerKeyboard.setAvailableRange (36, 96);
    lowerKeyboard.setScrollButtonsVisible (false);
    addAndMakeVisible (lowerKeyboard);

    pedalKeyboard.setName ("pedalKeys");
    pedalKeyboard.setOpaque (false);
    pedalKeyboard.setMidiChannel (3);
    pedalKeyboard.setMidiChannelsToDisplay (1 << 2);
    pedalKeyboard.setKeyWidth (20);
    pedalKeyboard.setAvailableRange (24, 49);
    pedalKeyboard.setScrollButtonsVisible (false);
    addAndMakeVisible (pedalKeyboard);
    
    for (auto p : proc.upperDrawBars)   addAndMakeVisible (controls.add (new Drawbar (p)));
    for (auto p : proc.lowerDrawBars)   addAndMakeVisible (controls.add (new Drawbar (p)));
    for (auto p : proc.pedalDrawBars)   addAndMakeVisible (controls.add (new Drawbar (p)));

    addAndMakeVisible (controls.add (new gin::Switch (proc.vibratoUpper)));
    addAndMakeVisible (controls.add (new gin::Switch (proc.vibratoLower)));
    addAndMakeVisible (controls.add (new gin::Select (proc.vibratoChorus)));
    addAndMakeVisible (controls.add (new gin::Select (proc.leslie)));
    addAndMakeVisible (controls.add (new gin::Switch (proc.prec)));
    addAndMakeVisible (controls.add (new gin::Switch (proc.precVol)));
    addAndMakeVisible (controls.add (new gin::Switch (proc.precDecay)));
    addAndMakeVisible (controls.add (new gin::Switch (proc.precHarmSel)));
    addAndMakeVisible (controls.add (new gin::Knob (proc.reverb)));
    addAndMakeVisible (controls.add (new gin::Knob (proc.volume)));
    addAndMakeVisible (controls.add (new gin::Switch (proc.overdrive)));
    addAndMakeVisible (controls.add (new gin::Knob (proc.character)));
    addAndMakeVisible (controls.add (new gin::Switch (proc.split)));

    setGridSize (15, 5);

#if JUCE_DEBUG
    layout.setLayout ({juce::File (__FILE__).getSiblingFile ("ui.json")});
#else
    layout.setLayout (juce::StringArray ("ui.json"));
#endif
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
