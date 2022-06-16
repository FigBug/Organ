#include "PluginProcessor.h"
#include "PluginEditor.h"

unsigned int defaultPreset[9] = { 8, 8, 6, 0, 0, 0, 0, 0, 0 };

//==============================================================================
OrganAudioProcessor::OrganAudioProcessor()
{
    for (int i = 0; i < 9; i++)
    {
        auto num = juce::String (i + 1);
        upperDrawBars[i] = addExtParam ("upper" + num, "Upper Draw Bar " + num, "Upper " + num, "", { 0.0f, 8.0f, 1.0f, 1.0f}, defaultPreset[i], 0.0f);
    }
    for (int i = 0; i < 9; i++)
    {
        auto num = juce::String (i + 1);
        lowerDrawBars[i] = addExtParam ("lower" + num, "Lower Draw Bar " + num, "Lower " + num, "", { 0.0f, 8.0f, 1.0f, 1.0f}, defaultPreset[i], 0.0f);
    }
    for (int i = 0; i < 2; i++)
    {
        auto num = juce::String (i + 1);
        pedalDrawBars[i] = addExtParam ("pedal" + num, "Pedal Draw Bar " + num, "Pedal " + num, "", { 0.0f, 8.0f, 1.0f, 1.0f}, defaultPreset[i], 0.0f);
    }

    vibratoUpper    = addExtParam ("vibratoUpper",    "Vibrate Upper",    "", "", { 0.0f, 1.0f, 1.0f, 1.0f}, 0.0f, 0.0f);
    vibratoLower    = addExtParam ("vibratoLower",    "Vibrate Lower",    "", "", { 0.0f, 1.0f, 1.0f, 1.0f}, 0.0f, 0.0f);
    vibratoChorus   = addExtParam ("vibratoChorus",   "Vibrate & Chorus", "", "", { 0.0f, 5.0f, 1.0f, 1.0f}, 0.0f, 0.0f);
    leslie          = addExtParam ("leslie",          "Leslie",           "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f);
    prec            = addExtParam ("prec",            "Perc",             "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f);
    precVol         = addExtParam ("precVol",         "Perc Volume",      "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f);
    precDecay       = addExtParam ("precDecay",       "Prec Decay",       "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f);
    precHarmSel     = addExtParam ("precHarmSel",     "Perc Harm Sel",    "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f);
    reverb          = addExtParam ("reverb",          "Reverb",           "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.0f, 0.0f);
    volume          = addExtParam ("volume",          "Volume",           "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.0f, 0.0f);
    overdrive       = addExtParam ("overdrive",       "Overdrive",        "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.0f, 0.0f);
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

    auto numSamples = buffer.getNumSamples();

    upperState.processNextMidiBuffer (midi, 0, numSamples, true);
    lowerState.processNextMidiBuffer (midi, 0, numSamples, true);
    pedalState.processNextMidiBuffer (midi, 0, numSamples, true);
    
    if (organ != nullptr)
    {
        for (int i = 0; i < 9; i++) organ->setUpperDrawBar (i, upperDrawBars[i]->getUserValueInt());
        for (int i = 0; i < 9; i++) organ->setLowerDrawBar (i, lowerDrawBars[i]->getUserValueInt());
        for (int i = 0; i < 2; i++) organ->setPedalDrawBar (i, pedalDrawBars[i]->getUserValueInt());

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
        organ->setOverdrive (overdrive->getUserValue());

        organ->processBlock (buffer, midi);
    }
}

//==============================================================================
bool OrganAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OrganAudioProcessor::createEditor()
{
    return new OrganAudioProcessorEditor (*this);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrganAudioProcessor();
}
