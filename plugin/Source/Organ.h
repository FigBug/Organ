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
    
private:
    void processMidi (juce::MidiBuffer& midi, int pos, int len);
    
    void allocAll();
    void initAll();
    void freeAll();
    
    double sampleRate = 44100.0;
    b_instance inst;
    
    gin::AudioFifo fifo { 2, 1024 };

    unsigned int upper[9] = { 0 };
    unsigned int lower[9] = { 0 };
    unsigned int pedal[9] = { 0 };
};
