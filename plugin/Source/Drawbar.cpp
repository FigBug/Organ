#include "Drawbar.h"

//==============================================================================
Drawbar::Drawbar (gin::Parameter* p)
  : ParamComponent (p),
    knob (parameter, juce::Slider::LinearVertical, juce::Slider::NoTextBox)
{
    addAndMakeVisible (knob);

    knob.setTitle (parameter->getName (100));
    knob.setDoubleClickReturnValue (true, parameter->getUserDefaultValue());
    knob.setSkewFactor (parameter->getSkew(), parameter->isSkewSymmetric());

    knob.setName (parameter->getShortName());

   #if JUCE_IOS
    knob.setMouseDragSensitivity (500);
   #endif

    addMouseListener (this, true);
}

Drawbar::~Drawbar()
{
}

void Drawbar::resized()
{
    knob.setBounds (getLocalBounds());
}

void Drawbar::parentHierarchyChanged()
{
    auto a = wantsAccessibleKeyboard (*this);
    knob.setWantsKeyboardFocus (a);
}
