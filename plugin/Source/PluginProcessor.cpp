#include "PluginProcessor.h"
#include "PluginEditor.h"

float defaultPreset[9] = { -8, -8, -6, 0, 0, 0, 0, 0, 0 };

static juce::String onOffTextFunction (const gin::Parameter&, float v)     { return v > 0.5f ? "On" : "Off"; }
static juce::String pVolTextFunction (const gin::Parameter&, float v)      { return v > 0.5f ? "Soft" : "Norm"; }
static juce::String pDecayTextFunction (const gin::Parameter&, float v)    { return v > 0.5f ? "Fast" : "Slow"; }
static juce::String pHarmTextFunction (const gin::Parameter&, float v)     { return v > 0.5f ? "2nd" : "3rd"; }
static juce::String percentTextFunction (const gin::Parameter&, float v)   { return juce::String (juce::roundToInt(v * 100)) + "%"; }

static juce::String vcTextFunction (const gin::Parameter&, float v)
{
    switch (juce::roundToInt (v))
    {
        case 0: return "V1";
        case 1: return "C1";
        case 2: return "V2";
        case 3: return "C2";
        case 4: return "V3";
        case 5: return "C3";
        default: return "";
    }
}

static juce::String lesTextFunction (const gin::Parameter&, float v)
{
    switch (juce::roundToInt (v))
    {
        case 0: return "Stop";
        case 1: return "Slow";
        case 2: return "Fast";
        default: return "";
    }
}

//==============================================================================
static gin::ProcessorOptions createProcessorOptions()
{
    return gin::ProcessorOptions()
        .withAdditionalCredits ({"Fredrik Kilander, Robin Gareus, Will Panther"})
        .withMidiLearn();
}

OrganAudioProcessor::OrganAudioProcessor()
    : gin::Processor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo()), false, createProcessorOptions())
{
    for (int i = 0; i < 9; i++)
    {
        auto num = juce::String (i + 1);
        upperDrawBars[i] = addExtParam ("upper" + num, "Upper Draw Bar " + num, "Upper " + num, "", { -8.0f, 0.0f, 1.0f, 1.0f}, defaultPreset[i], 0.0f);
    }
    for (int i = 0; i < 9; i++)
    {
        auto num = juce::String (i + 1);
        lowerDrawBars[i] = addExtParam ("lower" + num, "Lower Draw Bar " + num, "Lower " + num, "", { -8.0f, 0.0f, 1.0f, 1.0f}, defaultPreset[i], 0.0f);
    }
    for (int i = 0; i < 2; i++)
    {
        auto num = juce::String (i + 1);
        pedalDrawBars[i] = addExtParam ("pedal" + num, "Pedal Draw Bar " + num, "Pedal " + num, "", { -8.0f, 0.0f, 1.0f, 1.0f}, defaultPreset[i], 0.0f);
    }

    vibratoUpper    = addExtParam ("vibratoUpper",    "Vibrato Upper",    "", "", { 0.0f, 1.0f, 1.0f, 1.0f}, 0.0f, 0.0f, onOffTextFunction);
    vibratoLower    = addExtParam ("vibratoLower",    "Vibrato Lower",    "", "", { 0.0f, 1.0f, 1.0f, 1.0f}, 0.0f, 0.0f, onOffTextFunction);
    vibratoChorus   = addExtParam ("vibratoChorus",   "Vib & Chrs",       "", "", { 0.0f, 5.0f, 1.0f, 1.0f}, 0.0f, 0.0f, vcTextFunction);
    leslie          = addExtParam ("leslie",          "Leslie",           "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f, lesTextFunction);
    prec            = addExtParam ("prec",            "Perc",             "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 1.0f, 0.0f, onOffTextFunction);
    precVol         = addExtParam ("precVol",         "Perc Volume",      "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f, pVolTextFunction);
    precDecay       = addExtParam ("precDecay",       "Perc Decay",       "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f, pDecayTextFunction);
    precHarmSel     = addExtParam ("precHarmSel",     "Perc Harm Sel",    "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f, pHarmTextFunction);
    reverb          = addExtParam ("reverb",          "Reverb",           "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.2f, 0.0f, percentTextFunction);
    volume          = addExtParam ("volume",          "Volume",           "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 1.0f, 0.0f, percentTextFunction);
    overdrive       = addExtParam ("overdrive",       "Overdrive",        "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.0f, 0.0f, onOffTextFunction);
    character       = addExtParam ("character",       "Character",        "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.0f, 0.0f, percentTextFunction);
    split           = addExtParam ("split",           "Split Keys",       "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.0f, 0.0f, onOffTextFunction);

    midiOut.ensureSize (1024);
    init();
}

OrganAudioProcessor::~OrganAudioProcessor()
{
}

//==============================================================================
void OrganAudioProcessor::stateUpdated()
{
}

void OrganAudioProcessor::updateState()
{
}

//==============================================================================
void OrganAudioProcessor::reset()
{
    Processor::reset();
}

void OrganAudioProcessor::prepareToPlay (double newSampleRate, int newSamplesPerBlock)
{
    Processor::prepareToPlay (newSampleRate, newSamplesPerBlock);
    
    organ = std::make_unique<Organ> (newSampleRate, newSamplesPerBlock);
}

void OrganAudioProcessor::releaseResources()
{
}

void OrganAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    if (midiLearn)
        midiLearn->processBlock (midi, buffer.getNumSamples());

	buffer.clear ();
    auto numSamples = buffer.getNumSamples();

    if (organ != nullptr)
    {
        organ->preprocessMidi (midi, midiOut);

        upperState.processNextMidiBuffer (midiOut, 0, numSamples, true);
        lowerState.processNextMidiBuffer (midiOut, 0, numSamples, true);
        pedalState.processNextMidiBuffer (midiOut, 0, numSamples, true);

        for (int i = 0; i < 9; i++) organ->setUpperDrawBar (i, std::abs (upperDrawBars[i]->getUserValueInt()));
        for (int i = 0; i < 9; i++) organ->setLowerDrawBar (i, std::abs (lowerDrawBars[i]->getUserValueInt()));
        for (int i = 0; i < 2; i++) organ->setPedalDrawBar (i, std::abs (pedalDrawBars[i]->getUserValueInt()));

        organ->setSplit (split->getBoolValue());
        organ->setVibratoUpper (vibratoUpper->getUserValueBool());
        organ->setVibratoLower (vibratoLower->getUserValueBool());
        organ->setVibratoChorus (vibratoChorus->getUserValueInt());
        organ->setLeslie (leslie->getUserValueInt());
        organ->setPrec (prec->getUserValueBool());
        organ->setPrecVol (precVol->getUserValueBool());
        organ->setPrecDecay (precDecay->getUserValueBool());
        organ->setPrecHarmSel (precHarmSel->getUserValueBool());
        organ->setReverb (reverb->getUserValue());
        organ->setVolume (volume->getUserValue());
        organ->setOverdrive (overdrive->getUserValueBool());
        organ->setCharacter (character->getUserValueBool());

        organ->processBlock (buffer, midiOut);
    }
}

//==============================================================================
bool OrganAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OrganAudioProcessor::createEditor()
{
    return new gin::ScaledPluginEditor (new OrganAudioProcessorEditor (*this), state);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrganAudioProcessor();
}
