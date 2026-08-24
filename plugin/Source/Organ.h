#pragma once

#include <JuceHeader.h>

extern "C"
{
#include "../setBfree/src/global_inst.h"
}

class Organ
{
public:
    Organ (double sr, int bs, const juce::StringPairArray& config = {});
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

    // Backside (live) controls
    void setLeslieBypass (bool b);
    void setLeslieSpeeds (float hornSlow, float hornFast, float drumSlow, float drumFast);
    void setLeslieDynamics (float hornAcc, float hornDec, float drumAcc, float drumDec);
    void setLeslieBrakes (float horn, float drum);
    void setLeslieLevels (float horn, float leak);
    void setLeslieWidths (float horn, float drum);
    void setLeslieGeometry (float micDist, float micAngleDeg, float hornRadius, float drumRadius, float hornOffX, float hornOffZ);
    void setLeslieFilter (int which, int type, float hz, float q, float gain); // which: 0 = horn a, 1 = horn b, 2 = drum
    void setReverbInputGain (float g);

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

    float lesSpeeds[4]   = { -1, -1, -1, -1 };
    float lesDynamics[4] = { -1, -1, -1, -1 };
    float lesBrakes[2]   = { -1, -1 };
    float lesLevels[2]   = { -1, -1 };
    float lesWidths[2]   = { -2, -2 };
    float lesGeometry[6] = { -1, -1, -1, -1, -99, -99 };
    float lesFilters[3][4] = { { -1, -1, -1, -99 }, { -1, -1, -1, -99 }, { -1, -1, -1, -99 } };
    float revInputGain   = -1.0f;
};
