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
    
    void processBlock (AudioBuffer<float>&, MidiBuffer&);
    
private:
    void allocAll();
    void initAll();
    void freeAll();
    
    double sampleRate = 44100.0;
    b_instance inst;
    
    gin::AudioFifo fifo { 2, 1024 };
};
