#pragma once

#include <JuceHeader.h>
#include "Organ.h"

//==============================================================================
class OrganAudioProcessor : public gin::Processor,
                            private juce::AsyncUpdater
{
public:
    //==============================================================================
    OrganAudioProcessor();
    ~OrganAudioProcessor() override;

    void stateUpdated() override;
    void updateState() override;

    juce::File getProgramDirectory() override;
    juce::Array<juce::File> getFactoryProgramDirectories() override;

    //==============================================================================
    void reset() override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    std::unique_ptr<Organ> organ;

    gin::Parameter::Ptr upperDrawBars[9];
    gin::Parameter::Ptr lowerDrawBars[9];
    gin::Parameter::Ptr pedalDrawBars[2];
    gin::Parameter::Ptr vibratoUpper, vibratoLower, vibratoChorus, leslie, prec, precVol,
                        precDecay, precHarmSel, reverb, volume, overdrive, character, split;

    // Backside controls. The rebuild group only takes effect when the organ is
    // (re)initialized; changing one triggers an async rebuild of the Organ
    struct BacksideParams
    {
        // rebuild group
        gin::Parameter::Ptr tuning, temperament, transpose, transposeUpper, transposeLower, transposePedal;
        gin::Parameter::Ptr xtalkComp, xtalkTerm, xtalkWire, xtalkFloor, xtalkMin;
        gin::Parameter::Ptr attackModel, clickLevel, clickMin, clickMax, releaseModel, releaseLevel;
        gin::Parameter::Ptr scannerHz, scannerV1, scannerV2, scannerV3;
        gin::Parameter::Ptr percFast, percSlow, percNorm, percSoft, percGain;

        // live group
        gin::Parameter::Ptr lesBypass;
        gin::Parameter::Ptr hornSlow, hornFast, drumSlow, drumFast;
        gin::Parameter::Ptr hornAccel, hornDecel, drumAccel, drumDecel;
        gin::Parameter::Ptr hornBrake, drumBrake;
        gin::Parameter::Ptr hornLevel, hornLeak, hornWidth, drumWidth;
        gin::Parameter::Ptr micDist, micAngle, hornRadius, drumRadius, hornOffX, hornOffZ;
        gin::Parameter::Ptr hornF1Type, hornF1Hz, hornF1Q, hornF1Gain;
        gin::Parameter::Ptr hornF2Type, hornF2Hz, hornF2Q, hornF2Gain;
        gin::Parameter::Ptr drumFType, drumFHz, drumFQ, drumFGain;
        gin::Parameter::Ptr revGain;
    } backside;

    juce::MidiKeyboardState upperState;
    juce::MidiKeyboardState lowerState;
    juce::MidiKeyboardState pedalState;

    juce::MidiBuffer midiOut;

private:
    void addBacksideParams();
    juce::StringPairArray getOrganConfig() const;
    void applyBacksideLive();
    bool rebuildParamsChanged() const;

    void handleAsyncUpdate() override;

    std::vector<gin::Parameter::Ptr> rebuildParams;
    std::vector<float> organBuiltValues;    // rebuild-group values the current organ was built with
    std::vector<float> pendingBuiltValues;  // values the pending organ was built with

    std::atomic<Organ*> pendingOrgan { nullptr };
    std::atomic<Organ*> retiredOrgan { nullptr };
    std::atomic<bool> rebuildRequested { false };

    double lastSampleRate = 44100.0;
    int lastBlockSize = 512;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganAudioProcessor)
};
