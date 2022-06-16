#pragma once

#include <JuceHeader.h>

extern "C"
{
#include "../setBfree/src/global_inst.h"
}

class Organ
{
public:
    Organ (double sr, int bs);
    ~Organ();
    
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&);

    void setUpperDrawBar (int idx, int val);
    void setLowerDrawBar (int idx, int val);
    void setPedalDrawBar (int idx, int val);

    void preprocessMidi (juce::MidiBuffer& src, juce::MidiBuffer& dst);
    void setVibratoUpper (bool v);
    void setVibratoLower (bool v);
    void setVibratoChorus (int v);
    void setLeslie (int v);
    void setPrec (bool v);
    void setPrecVol (bool v);
    void setPrecDecay (bool v);
    void setPrecHarmSel (bool v);
    void setReverb (float v);
    void setVolume (float v);
    void setOverdrive (bool v);
    void setCharacter (float f);
    void setSplit (bool s);

private:
    void processMidi (juce::MidiBuffer& midi, int pos, int len);

    void allocAll();
    void initAll();
    void freeAll();

    double sampleRate = 44100.0;
    b_instance inst;
    
    gin::AudioFifo fifo { 2, 1024 };

    bool upperDirty = false;
    bool lowerDirty = false;
    bool pedalDirty = false;

    unsigned int upper[9] = { 0 };
    unsigned int lower[9] = { 0 };
    unsigned int pedal[9] = { 0 };

    int leslie = -1;
    bool split = false;
};
