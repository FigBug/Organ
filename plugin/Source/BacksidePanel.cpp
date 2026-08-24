#include "BacksidePanel.h"

//==============================================================================
BacksidePanel::BacksidePanel (OrganAudioProcessor& p)
    : proc (p)
{
    auto& b = proc.backside;

    auto knob   = [this] (gin::Parameter::Ptr p2) -> juce::Component* { return controls.add (new gin::Knob (p2)); };
    auto select = [this] (gin::Parameter::Ptr p2) -> juce::Component* { return controls.add (new gin::Select (p2)); };
    auto toggle = [this] (gin::Parameter::Ptr p2) -> juce::Component* { return controls.add (new gin::Switch (p2)); };

    addSection ("Tuning",
                { knob (b.tuning), select (b.temperament), knob (b.transpose),
                  knob (b.transposeUpper), knob (b.transposeLower), knob (b.transposePedal) });

    addSection ("Tonewheels",
                { knob (b.xtalkComp), knob (b.xtalkTerm), knob (b.xtalkWire),
                  knob (b.xtalkFloor), knob (b.xtalkMin) });

    addSection ("Key Click",
                { select (b.attackModel), knob (b.clickLevel), knob (b.clickMin), knob (b.clickMax),
                  select (b.releaseModel), knob (b.releaseLevel) });

    addSection ("Vibrato Scanner",
                { knob (b.scannerHz), knob (b.scannerV1), knob (b.scannerV2), knob (b.scannerV3) });

    addSection ("Percussion",
                { knob (b.percFast), knob (b.percSlow), knob (b.percNorm),
                  knob (b.percSoft), knob (b.percGain) });

    addSection ("Reverb",
                { knob (proc.backside.revGain) });

    addSection ("Leslie Motors",
                { toggle (b.lesBypass),
                  knob (b.hornSlow), knob (b.hornFast), knob (b.hornAccel), knob (b.hornDecel), knob (b.hornBrake),
                  knob (b.drumSlow), knob (b.drumFast), knob (b.drumAccel), knob (b.drumDecel), knob (b.drumBrake) });

    addSection ("Leslie Speaker",
                { knob (b.hornLevel), knob (b.hornLeak), knob (b.hornWidth), knob (b.drumWidth),
                  knob (b.micDist), knob (b.micAngle), knob (b.hornRadius), knob (b.drumRadius),
                  knob (b.hornOffX), knob (b.hornOffZ) });

    addSection ("Leslie Filters",
                { select (b.hornF1Type), knob (b.hornF1Hz), knob (b.hornF1Q), knob (b.hornF1Gain),
                  select (b.hornF2Type), knob (b.hornF2Hz), knob (b.hornF2Q), knob (b.hornF2Gain),
                  select (b.drumFType), knob (b.drumFHz), knob (b.drumFQ), knob (b.drumFGain) });
}

void BacksidePanel::addSection (const juce::String& name, std::initializer_list<juce::Component*> comps)
{
    auto header = headers.add (new juce::Label ({}, name));
    header->setJustificationType (juce::Justification::centredLeft);
    header->setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    addAndMakeVisible (header);

    Section s;
    s.header = header;
    for (auto c : comps)
    {
        addAndMakeVisible (c);
        s.comps.add (c);
    }
    sections.push_back (std::move (s));
}

int BacksidePanel::doLayout (int width, bool apply)
{
    const int cw = 56, ch = 70, gap = 4, margin = 8;
    int y = margin;

    for (auto& s : sections)
    {
        if (apply)
            s.header->setBounds (margin, y, width - margin * 2, 18);
        y += 22;

        int x = margin;
        for (auto c : s.comps)
        {
            if (x + cw > width - margin)
            {
                x = margin;
                y += ch + gap;
            }
            if (apply)
                c->setBounds (x, y, cw, ch);
            x += cw + gap;
        }
        y += ch + gap + margin;
    }

    return y;
}

int BacksidePanel::heightForWidth (int w)
{
    return doLayout (w, false);
}

void BacksidePanel::paint (juce::Graphics& g)
{
    juce::ColourGradient grad (findColour (gin::PluginLookAndFeel::matte1ColourId), 0, 0,
                               findColour (gin::PluginLookAndFeel::matte2ColourId), 0, float (getHeight()), false);
    g.setGradientFill (grad);
    g.fillAll();
}

void BacksidePanel::resized()
{
    doLayout (getWidth(), true);
}
