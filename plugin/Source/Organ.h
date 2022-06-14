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
    
private:
    void processMidi (juce::MidiBuffer& midi, int pos, int len);
    
    void allocAll();
    void initAll();
    void freeAll();
    
    double sampleRate = 44100.0;
    b_instance inst;
    
    gin::AudioFifo fifo { 2, 1024 };
};
