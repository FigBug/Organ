#pragma once

#include <JuceHeader.h>

//==============================================================================
class Drawbar : public gin::ParamComponent
{
public:
    Drawbar (gin::Parameter* parameter);
    ~Drawbar() override;

    gin::PluginSlider& getSlider()   { return knob; }

private:
    void resized() override;
    void parentHierarchyChanged() override;

    gin::PluginSlider knob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Drawbar)
};
