#include "Organ.h"

extern "C"
{
#include "../setBfree/src/state.h"
#include "../setBfree/src/tonegen.h"
#include "../setBfree/src/midi.h"
}

Organ::Organ (double sr, int bs)
    : sampleRate (sr)
{
    fifo.setSize (2, std::max (1024, bs * 2));
    allocAll();
    initAll();
}

Organ::~Organ()
{
    freeAll();
}

void Organ::allocAll()
{
    inst.state   = allocRunningConfig();
    inst.progs   = allocProgs();
    inst.reverb  = allocReverb();
    inst.whirl   = allocWhirl();
    inst.synth   = allocTonegen();
    inst.midicfg = allocMidiCfg (inst.state);
    inst.preamp  = allocPreamp();
}

void Organ::freeAll()
{
    freeReverb (inst.reverb);
    freeWhirl (inst.whirl);

    freeToneGenerator (inst.synth);
    freeMidiCfg (inst.midicfg);
    freePreamp (inst.preamp);
    freeProgs (inst.progs);
    freeRunningConfig (inst.state);
}

void Organ::initAll()
{
    initToneGenerator (inst.synth, inst.midicfg, sampleRate);
    initVibrato (inst.synth, inst.midicfg, sampleRate);
    initPreamp (inst.preamp, inst.midicfg);
    initReverb (inst.reverb, inst.midicfg, sampleRate);
    initWhirl (inst.whirl, inst.midicfg, sampleRate);
    initRunningConfig (inst.state, inst.midicfg);
    
    initMidiTables (inst.midicfg);
    
    unsigned int defaultPreset[9] = { 8, 8, 6, 0, 0, 0, 0, 0, 0 };
    setDrawBars (&inst, 0, defaultPreset);
}

void Organ::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    MidiMessage msg;
    int pos = 0;
    MidiBuffer::Iterator itr (midi);
    
    while (itr.getNextEvent (msg, pos))
    {
        auto data = msg.getRawData();
        auto size = msg.getRawDataSize();
        
        parse_raw_midi_data (&inst, data, size);
    }
    
    while (fifo.getNumReady() < buffer.getNumSamples())
    {
        gin::ScratchBuffer temp {5, BUFFER_SIZE_SAMPLES};
        gin::ScratchBuffer out {2, BUFFER_SIZE_SAMPLES};
        auto a = temp.getWritePointer (0);
        auto b = temp.getWritePointer (1);
        auto c = temp.getWritePointer (2);
        auto t = temp.getWritePointer (3);
        auto u = temp.getWritePointer (4);
        
        auto l = out.getWritePointer (0);
        auto r = out.getWritePointer (1);

        oscGenerateFragment (inst.synth, a, BUFFER_SIZE_SAMPLES);
        preamp (inst.preamp, a, b, BUFFER_SIZE_SAMPLES);
        reverb (inst.reverb, b, c, BUFFER_SIZE_SAMPLES);
        
        whirlProc3 (inst.whirl, c, l, r, t, u, BUFFER_SIZE_SAMPLES);
        
        fifo.write (out);
    }
    
    fifo.read (buffer);
}
