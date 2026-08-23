#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <mutex>

float defaultPreset[9] = { -8, -8, -6, 0, 0, 0, 0, 0, 0 };

// Builds a conversion function for a two state parameter: values above 0.5 display
// as onText, others as offText, and typed text maps back to 1 or 0
static gin::Parameter::ConversionFunction switchConversion (juce::String offText, juce::String onText)
{
    return [offText, onText] (const gin::Parameter&, const std::variant<float, juce::String>& v) -> std::variant<float, juce::String>
    {
        if (auto f = std::get_if<float> (&v))
            return *f > 0.5f ? onText : offText;

        auto t = std::get<juce::String> (v).trim();
        if (t.equalsIgnoreCase (onText))  return 1.0f;
        if (t.equalsIgnoreCase (offText)) return 0.0f;
        return t.getFloatValue();
    };
}

// Builds a conversion function for a choice parameter: the value is an index into
// names, and typed text maps back to the index of the matching name
static gin::Parameter::ConversionFunction choicesConversion (juce::StringArray names)
{
    return [names] (const gin::Parameter&, const std::variant<float, juce::String>& v) -> std::variant<float, juce::String>
    {
        if (auto f = std::get_if<float> (&v))
        {
            auto idx = juce::roundToInt (*f);
            return juce::isPositiveAndBelow (idx, names.size()) ? names[idx] : juce::String();
        }

        auto t = std::get<juce::String> (v).trim();
        for (int i = 0; i < names.size(); i++)
            if (t.equalsIgnoreCase (names[i]))
                return float (i);
        return t.getFloatValue();
    };
}

static std::variant<float, juce::String> percentConversionFunction (const gin::Parameter&, const std::variant<float, juce::String>& v)
{
    if (auto f = std::get_if<float> (&v))
        return juce::String (juce::roundToInt (*f * 100)) + "%";

    return std::get<juce::String> (v).getFloatValue() / 100.0f;
}

//==============================================================================
static gin::ProcessorOptions createProcessorOptions()
{
    return gin::ProcessorOptions()
        .withAdditionalCredits ({"Fredrik Kilander, Robin Gareus, Will Panther"})
        .withMidiLearn();
}

// If the shared CrashReporter is installed, launch it once per process (on the
// first plugin instance) so it can scan and upload any crash from last session.
static void launchCrashReporterOnce()
{
    static std::once_flag flag;
    std::call_once (flag, []
    {
       #if JUCE_MAC
        juce::File app ("/Library/Application Support/Rabien Software/Crash Reporter/CrashReporter.app");
       #elif JUCE_WINDOWS
        auto app = juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory)
                       .getChildFile ("Rabien Software").getChildFile ("Crash Reporter").getChildFile ("CrashReporter.exe");
       #else
        juce::File app;
       #endif

        if (app.exists())
            juce::Process::openDocument (app.getFullPathName(), {});
    });
}

OrganAudioProcessor::OrganAudioProcessor()
    : gin::Processor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo()), false, createProcessorOptions())
{
    launchCrashReporterOnce();

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

    vibratoUpper    = addExtParam ("vibratoUpper",    "Vibrato Upper",    "", "", { 0.0f, 1.0f, 1.0f, 1.0f}, 0.0f, 0.0f, switchConversion ("Off", "On"));
    vibratoLower    = addExtParam ("vibratoLower",    "Vibrato Lower",    "", "", { 0.0f, 1.0f, 1.0f, 1.0f}, 0.0f, 0.0f, switchConversion ("Off", "On"));
    vibratoChorus   = addExtParam ("vibratoChorus",   "Vib & Chrs",       "", "", { 0.0f, 5.0f, 1.0f, 1.0f}, 0.0f, 0.0f, choicesConversion ({ "V1", "C1", "V2", "C2", "V3", "C3" }));
    leslie          = addExtParam ("leslie",          "Leslie",           "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f, choicesConversion ({ "Stop", "Slow", "Fast" }));
    prec            = addExtParam ("prec",            "Perc",             "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 1.0f, 0.0f, switchConversion ("Off", "On"));
    precVol         = addExtParam ("precVol",         "Perc Volume",      "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f, switchConversion ("Norm", "Soft"));
    precDecay       = addExtParam ("precDecay",       "Perc Decay",       "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f, switchConversion ("Slow", "Fast"));
    precHarmSel     = addExtParam ("precHarmSel",     "Perc Harm Sel",    "", "", { 0.0f, 2.0f, 1.0f, 1.0f}, 0.0f, 0.0f, switchConversion ("3rd", "2nd"));
    reverb          = addExtParam ("reverb",          "Reverb",           "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.2f, 0.0f, percentConversionFunction);
    volume          = addExtParam ("volume",          "Volume",           "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 1.0f, 0.0f, percentConversionFunction);
    overdrive       = addExtParam ("overdrive",       "Overdrive",        "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.0f, 0.0f, switchConversion ("Off", "On"));
    character       = addExtParam ("character",       "Character",        "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.0f, 0.0f, percentConversionFunction);
    split           = addExtParam ("split",           "Split Keys",       "", "", { 0.0f, 1.0f, 0.0f, 1.0f}, 0.0f, 0.0f, switchConversion ("Off", "On"));

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
