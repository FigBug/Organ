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
}

void Organ::setUpperDrawBar (int idx, int val)
{
    upper[idx] = (unsigned int)val;
}

void Organ::setLowerDrawBar (int idx, int val)
{
    upper[idx] = (unsigned int)val;
}

void Organ::setPedalDrawBar (int idx, int val)
{
    pedal[idx] = (unsigned int)val;
}

void Organ::processMidi (juce::MidiBuffer& midi, int pos, int len)
{
    for (const juce::MidiMessageMetadata metadata : midi)
    {
        if (metadata.samplePosition < pos)
            continue;
        if (metadata.samplePosition >= pos + len)
            break;
        
        auto data = metadata.data;
        auto size = metadata.numBytes;
        
        parse_raw_midi_data (&inst, data, size_t (size));
    }
}

void Organ::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const int stepSize = BUFFER_SIZE_SAMPLES;
    int pos = 0;

    setDrawBars (&inst, 0, upper);
    setDrawBars (&inst, 1, lower);
    setDrawBars (&inst, 2, pedal);
    
    while (fifo.getNumReady() < buffer.getNumSamples())
    {
        processMidi (midi, pos, stepSize);
        
        gin::ScratchBuffer temp {5, stepSize};
        gin::ScratchBuffer out {2, stepSize};
        auto a = temp.getWritePointer (0);
        auto b = temp.getWritePointer (1);
        auto c = temp.getWritePointer (2);
        auto t = temp.getWritePointer (3);
        auto u = temp.getWritePointer (4);
        
        auto l = out.getWritePointer (0);
        auto r = out.getWritePointer (1);

        oscGenerateFragment (inst.synth, a, stepSize);
        preamp (inst.preamp, a, b, stepSize);
        reverb (inst.reverb, b, c, stepSize);
        
        whirlProc3 (inst.whirl, c, l, r, t, u, stepSize);
        
        fifo.write (out);
        pos += stepSize;
    }
    
    if (pos < buffer.getNumSamples())
        processMidi (midi, pos, buffer.getNumSamples() - pos);
    
    fifo.read (buffer);
}
