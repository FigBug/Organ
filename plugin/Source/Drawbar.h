#pragma once

#include <JuceHeader.h>



//==============================================================================
class Drawbar : public gin::ParamComponent
{
public:
    Drawbar (gin::Parameter* parameter);
    ~Drawbar() override;

private:
    void resized() override;
    void parentHierarchyChanged() override;

    class DrawBarSlider : public gin::PluginSlider
    {
    public:
        DrawBarSlider (gin::Parameter* p, SliderStyle s, TextEntryBoxPosition t)
            : PluginSlider (p, s, t)
        {
        }

        void paint (juce::Graphics& g) override
        {
            juce::Rectangle<int> rc (0, 0, getWidth(), getHeight());
            rc.removeFromBottom (getWidth());

            auto numberHeight = rc.getHeight() / 8.0f;

            // Frame
            {
                rc = rc.withHeight (int (rc.getHeight() * (1.0f - valueToProportionOfLength (getValue()))));

                g.setColour (findColour (gin::PluginLookAndFeel::accentColourId).withAlpha (0.5f));
                g.drawRect (rc);
            }

            // Numbers
            {
                auto nt = rc.toFloat();
                nt.setTop (-1000.0);

                g.setFont (8.0f);
                g.setColour (findColour (gin::PluginLookAndFeel::accentColourId).withAlpha (0.8f));

                for (int i = 1; i <= 8; i++)
                    g.drawText (juce::String (i), nt.removeFromBottom (numberHeight), juce::Justification::centred);
            }

            // Knob
            {
                juce::Graphics::ScopedSaveState sss (g);
                g.setColour (getColour (getName().getTrailingIntValue() - 1));
                g.reduceClipRegion (0, rc.getBottom(), rc.getWidth(), rc.getWidth());
                g.fillEllipse (0, rc.getBottom() - rc.getWidth() / 2, rc.getWidth(), rc.getWidth());
            }
        }

        juce::Colour getColour (int idx)
        {
            switch (idx)
            {
                case 0:
                case 1:
                    return findColour (gin::PluginLookAndFeel::accentColourId).withAlpha (0.9f);
                case 2:
                case 3:
                case 5:
                case 8:
                    return findColour (gin::PluginLookAndFeel::whiteColourId).withAlpha (0.9f);
                default:
                    return findColour (gin::PluginLookAndFeel::blackColourId).withAlpha (0.9f);
            }
        }
    };

    DrawBarSlider knob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Drawbar)
};
