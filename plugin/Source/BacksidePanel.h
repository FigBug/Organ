#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// The organ's "backside": the setBfree config controls, shown as an overlay
class BacksidePanel : public juce::Component
{
public:
    BacksidePanel (OrganAudioProcessor&);

    int heightForWidth (int w);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Section
    {
        juce::Label* header = nullptr;
        juce::Array<juce::Component*> comps;
    };

    void addSection (const juce::String& name, std::initializer_list<juce::Component*> comps);
    int doLayout (int width, bool apply);

    OrganAudioProcessor& proc;

    juce::OwnedArray<juce::Component> controls;
    juce::OwnedArray<juce::Label> headers;
    std::vector<Section> sections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BacksidePanel)
};
