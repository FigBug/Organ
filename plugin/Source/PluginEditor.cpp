#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Drawbar.h"

// Material Design "settings" gear icon
static const char* gearSVGPath =
    "M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22"
    "l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54"
    "c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58"
    "c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96"
    "c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94"
    "l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58z"
    "M12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z";

//==============================================================================
OrganAudioProcessorEditor::OrganAudioProcessorEditor (OrganAudioProcessor& p_)
    : ProcessorEditor (p_), proc (p_), gearButton ("gear", gearSVGPath)
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

    backsideViewport.setViewedComponent (&backsidePanel, false);
    backsideViewport.setScrollBarsShown (true, false);
    backsideViewport.setScrollBarThickness (8);
    addChildComponent (backsideViewport);

    gearButton.setName ("gear");
    gearButton.onClick = [this]
    {
        if (! backsideViewport.isVisible())
            backsideViewport.setViewPosition (0, 0);
        backsideViewport.setVisible (! backsideViewport.isVisible());
    };
    addAndMakeVisible (gearButton);

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

    auto content = getLocalBounds().withTrimmedTop (40);
    backsideViewport.setBounds (content);

    auto panelWidth = content.getWidth() - backsideViewport.getScrollBarThickness();
    backsidePanel.setSize (panelWidth, std::max (content.getHeight(), backsidePanel.heightForWidth (panelWidth)));

    gearButton.setBounds (getWidth() - 26, getHeight() - 26, 22, 22);
    gearButton.toFront (false);
}
