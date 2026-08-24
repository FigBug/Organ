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

// Displays a linear gain value as dB, and typed dB text maps back to linear gain
static std::variant<float, juce::String> dbConversionFunction (const gin::Parameter&, const std::variant<float, juce::String>& v)
{
    if (auto f = std::get_if<float> (&v))
    {
        if (*f <= 0.0f)
            return juce::String ("-inf");
        return juce::String (juce::Decibels::gainToDecibels (*f), 1) + " dB";
    }

    auto t = std::get<juce::String> (v).trim();
    if (t.startsWithIgnoreCase ("-inf"))
        return 0.0f;
    return juce::Decibels::decibelsToGain (t.getFloatValue());
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

    addBacksideParams();

    midiOut.ensureSize (1024);
    init();
}

OrganAudioProcessor::~OrganAudioProcessor()
{
    cancelPendingUpdate();
    delete pendingOrgan.exchange (nullptr);
    delete retiredOrgan.exchange (nullptr);
}

void OrganAudioProcessor::addBacksideParams()
{
    auto& b = backside;

    juce::StringArray temperaments { "Gear60", "Gear50", "Equal" };
    juce::StringArray models { "Click", "Shelf", "Cosine", "Linear" };
    juce::StringArray filterTypes { "Low Pass", "High Pass", "Band Pass 1", "Band Pass 2", "Notch", "All Pass", "Peaking", "Low Shelf", "High Shelf" };

    // Rebuild group: defaults and ranges from the setBfree ConfigDoc tables
    b.tuning         = addExtParam ("tuning",         "Tuning",           "",         "Hz",  { 220.0f, 880.0f, 0.0f, 1.0f }, 440.0f, 0.0f);
    b.temperament    = addExtParam ("temperament",    "Temperament",      "",         "",    { 0.0f, 2.0f, 1.0f, 1.0f }, 0.0f, 0.0f, choicesConversion (temperaments));
    b.transpose      = addExtParam ("transpose",      "Transpose",        "",         "st",  { -12.0f, 12.0f, 1.0f, 1.0f }, 0.0f, 0.0f);
    b.transposeUpper = addExtParam ("transposeUpper", "Transpose Upper",  "Tr Upper", "st",  { -12.0f, 12.0f, 1.0f, 1.0f }, 0.0f, 0.0f);
    b.transposeLower = addExtParam ("transposeLower", "Transpose Lower",  "Tr Lower", "st",  { -12.0f, 12.0f, 1.0f, 1.0f }, 0.0f, 0.0f);
    b.transposePedal = addExtParam ("transposePedal", "Transpose Pedal",  "Tr Pedal", "st",  { -12.0f, 12.0f, 1.0f, 1.0f }, 0.0f, 0.0f);

    b.xtalkComp      = addExtParam ("xtalkComp",      "Compartment X-Talk",    "Comp",  "", { 0.0f, 0.5f, 0.0f, 0.3f }, 0.01f, 0.0f, dbConversionFunction);
    b.xtalkTerm      = addExtParam ("xtalkTerm",      "Terminal Strip X-Talk", "Term",  "", { 0.0f, 0.5f, 0.0f, 0.3f }, 0.01f, 0.0f, dbConversionFunction);
    b.xtalkWire      = addExtParam ("xtalkWire",      "Wiring X-Talk",         "Wire",  "", { 0.0f, 0.5f, 0.0f, 0.3f }, 0.01f, 0.0f, dbConversionFunction);
    b.xtalkFloor     = addExtParam ("xtalkFloor",     "X-Talk Floor",          "Floor", "", { 0.0f, 0.001f, 0.0f, 0.3f }, 0.0000158f, 0.0f, dbConversionFunction);
    b.xtalkMin       = addExtParam ("xtalkMin",       "X-Talk Min",            "Min",   "", { 0.0f, 0.001f, 0.0f, 0.3f }, 0.0f, 0.0f, dbConversionFunction);

    b.attackModel    = addExtParam ("attackModel",    "Attack Model",     "Attack",   "",  { 0.0f, 3.0f, 1.0f, 1.0f }, 0.0f, 0.0f, choicesConversion (models));
    b.clickLevel     = addExtParam ("clickLevel",     "Key Click Level",  "Click",    "",  { 0.0f, 1.0f, 0.0f, 1.0f }, 0.5f, 0.0f, percentConversionFunction);
    b.clickMin       = addExtParam ("clickMin",       "Click Min Length", "Click Mn", "",  { 0.0f, 1.0f, 0.0f, 1.0f }, 0.125f, 0.0f, percentConversionFunction);
    b.clickMax       = addExtParam ("clickMax",       "Click Max Length", "Click Mx", "",  { 0.0f, 1.0f, 0.0f, 1.0f }, 0.625f, 0.0f, percentConversionFunction);
    b.releaseModel   = addExtParam ("releaseModel",   "Release Model",    "Release",  "",  { 0.0f, 3.0f, 1.0f, 1.0f }, 3.0f, 0.0f, choicesConversion (models));
    b.releaseLevel   = addExtParam ("releaseLevel",   "Release Level",    "Rel Lvl",  "",  { 0.0f, 1.0f, 0.0f, 1.0f }, 0.25f, 0.0f, percentConversionFunction);

    b.scannerHz      = addExtParam ("scannerHz",      "Scanner Freq",     "Freq",     "Hz", { 4.0f, 22.0f, 0.0f, 1.0f }, 7.25f, 0.0f);
    b.scannerV1      = addExtParam ("scannerV1",      "Vibrato 1 Mod",    "V1 Mod",   "Hz", { 0.0f, 12.0f, 0.0f, 1.0f }, 3.0f, 0.0f);
    b.scannerV2      = addExtParam ("scannerV2",      "Vibrato 2 Mod",    "V2 Mod",   "Hz", { 0.0f, 12.0f, 0.0f, 1.0f }, 6.0f, 0.0f);
    b.scannerV3      = addExtParam ("scannerV3",      "Vibrato 3 Mod",    "V3 Mod",   "Hz", { 0.0f, 12.0f, 0.0f, 1.0f }, 9.0f, 0.0f);

    b.percFast       = addExtParam ("percFast",       "Perc Fast Decay",  "Fast",     "s",  { 0.1f, 10.0f, 0.0f, 0.5f }, 1.0f, 0.0f);
    b.percSlow       = addExtParam ("percSlow",       "Perc Slow Decay",  "Slow",     "s",  { 0.1f, 10.0f, 0.0f, 0.5f }, 4.0f, 0.0f);
    b.percNorm       = addExtParam ("percNorm",       "Perc Norm Level",  "Norm",     "",   { 0.0f, 1.0f, 0.0f, 1.0f }, 1.0f, 0.0f, dbConversionFunction);
    b.percSoft       = addExtParam ("percSoft",       "Perc Soft Level",  "Soft",     "",   { 0.0f, 0.89125f, 0.0f, 1.0f }, 0.5012f, 0.0f, dbConversionFunction);
    b.percGain       = addExtParam ("percGain",       "Perc Gain",        "Gain",     "",   { 0.0f, 22.0f, 0.0f, 1.0f }, 11.0f, 0.0f);

    // Live group: applied to the running organ
    b.lesBypass      = addExtParam ("lesBypass",      "Leslie Bypass",    "Bypass",   "",    { 0.0f, 1.0f, 1.0f, 1.0f }, 0.0f, 0.0f, switchConversion ("Off", "On"));
    b.hornSlow       = addExtParam ("hornSlow",       "Horn Slow",        "",         "RPM", { 5.0f, 200.0f, 0.0f, 1.0f }, 40.32f, 0.0f);
    b.hornFast       = addExtParam ("hornFast",       "Horn Fast",        "",         "RPM", { 100.0f, 900.0f, 0.0f, 1.0f }, 423.36f, 0.0f);
    b.drumSlow       = addExtParam ("drumSlow",       "Drum Slow",        "",         "RPM", { 5.0f, 100.0f, 0.0f, 1.0f }, 36.0f, 0.0f);
    b.drumFast       = addExtParam ("drumFast",       "Drum Fast",        "",         "RPM", { 60.0f, 600.0f, 0.0f, 1.0f }, 357.3f, 0.0f);
    b.hornAccel      = addExtParam ("hornAccel",      "Horn Accel",       "",         "s",   { 0.05f, 2.0f, 0.0f, 0.5f }, 0.161f, 0.0f);
    b.hornDecel      = addExtParam ("hornDecel",      "Horn Decel",       "",         "s",   { 0.05f, 2.0f, 0.0f, 0.5f }, 0.321f, 0.0f);
    b.drumAccel      = addExtParam ("drumAccel",      "Drum Accel",       "",         "s",   { 0.5f, 10.0f, 0.0f, 0.5f }, 4.127f, 0.0f);
    b.drumDecel      = addExtParam ("drumDecel",      "Drum Decel",       "",         "s",   { 0.5f, 10.0f, 0.0f, 0.5f }, 1.371f, 0.0f);
    b.hornBrake      = addExtParam ("hornBrake",      "Horn Brake",       "",         "",    { 0.0f, 1.0f, 0.0f, 1.0f }, 0.0f, 0.0f);
    b.drumBrake      = addExtParam ("drumBrake",      "Drum Brake",       "",         "",    { 0.0f, 1.0f, 0.0f, 1.0f }, 0.0f, 0.0f);

    b.hornLevel      = addExtParam ("hornLevel",      "Horn Level",       "",         "",    { 0.0f, 1.0f, 0.0f, 1.0f }, 0.7f, 0.0f, dbConversionFunction);
    b.hornLeak       = addExtParam ("hornLeak",       "Horn Leak",        "",         "",    { 0.0f, 1.0f, 0.0f, 1.0f }, 0.15f, 0.0f, dbConversionFunction);
    b.hornWidth      = addExtParam ("hornWidth",      "Horn Width",       "",         "",    { -1.0f, 1.0f, 0.0f, 1.0f }, 0.0f, 0.0f);
    b.drumWidth      = addExtParam ("drumWidth",      "Drum Width",       "",         "",    { -1.0f, 1.0f, 0.0f, 1.0f }, 0.0f, 0.0f);
    b.micDist        = addExtParam ("micDist",        "Mic Distance",     "Mic Dist", "cm",  { 9.0f, 100.0f, 0.0f, 1.0f }, 42.0f, 0.0f);
    b.micAngle       = addExtParam ("micAngle",       "Mic Angle",        "",         "deg", { 0.0f, 180.0f, 0.0f, 1.0f }, 180.0f, 0.0f);
    b.hornRadius     = addExtParam ("hornRadius",     "Horn Radius",      "",         "cm",  { 9.0f, 50.0f, 0.0f, 1.0f }, 19.2f, 0.0f);
    b.drumRadius     = addExtParam ("drumRadius",     "Drum Radius",      "",         "cm",  { 9.0f, 50.0f, 0.0f, 1.0f }, 22.0f, 0.0f);
    b.hornOffX       = addExtParam ("hornOffX",       "Horn X Offset",    "Horn X",   "cm",  { -20.0f, 20.0f, 0.0f, 1.0f }, 0.0f, 0.0f);
    b.hornOffZ       = addExtParam ("hornOffZ",       "Horn Z Offset",    "Horn Z",   "cm",  { -20.0f, 20.0f, 0.0f, 1.0f }, 0.0f, 0.0f);

    b.hornF1Type     = addExtParam ("hornF1Type",     "Horn F1 Type",     "F1 Type",  "",    { 0.0f, 8.0f, 1.0f, 1.0f }, 0.0f, 0.0f, choicesConversion (filterTypes));
    b.hornF1Hz       = addExtParam ("hornF1Hz",       "Horn F1 Freq",     "F1 Freq",  "Hz",  { 20.0f, 8000.0f, 0.0f, 0.3f }, 4500.0f, 0.0f);
    b.hornF1Q        = addExtParam ("hornF1Q",        "Horn F1 Q",        "F1 Q",     "",    { 0.1f, 6.0f, 0.0f, 0.5f }, 2.7456f, 0.0f);
    b.hornF1Gain     = addExtParam ("hornF1Gain",     "Horn F1 Gain",     "F1 Gain",  "dB",  { -48.0f, 48.0f, 0.0f, 1.0f }, -30.0f, 0.0f);
    b.hornF2Type     = addExtParam ("hornF2Type",     "Horn F2 Type",     "F2 Type",  "",    { 0.0f, 8.0f, 1.0f, 1.0f }, 7.0f, 0.0f, choicesConversion (filterTypes));
    b.hornF2Hz       = addExtParam ("hornF2Hz",       "Horn F2 Freq",     "F2 Freq",  "Hz",  { 20.0f, 8000.0f, 0.0f, 0.3f }, 300.0f, 0.0f);
    b.hornF2Q        = addExtParam ("hornF2Q",        "Horn F2 Q",        "F2 Q",     "",    { 0.1f, 6.0f, 0.0f, 0.5f }, 1.0f, 0.0f);
    b.hornF2Gain     = addExtParam ("hornF2Gain",     "Horn F2 Gain",     "F2 Gain",  "dB",  { -48.0f, 48.0f, 0.0f, 1.0f }, -30.0f, 0.0f);
    b.drumFType      = addExtParam ("drumFType",      "Drum Filt Type",   "Dr Type",  "",    { 0.0f, 8.0f, 1.0f, 1.0f }, 8.0f, 0.0f, choicesConversion (filterTypes));
    b.drumFHz        = addExtParam ("drumFHz",        "Drum Filt Freq",   "Dr Freq",  "Hz",  { 20.0f, 8000.0f, 0.0f, 0.3f }, 811.9695f, 0.0f);
    b.drumFQ         = addExtParam ("drumFQ",         "Drum Filt Q",      "Dr Q",     "",    { 0.1f, 6.0f, 0.0f, 0.5f }, 1.6016f, 0.0f);
    b.drumFGain      = addExtParam ("drumFGain",      "Drum Filt Gain",   "Dr Gain",  "dB",  { -48.0f, 48.0f, 0.0f, 1.0f }, -38.9291f, 0.0f);

    b.revGain        = addExtParam ("revGain",        "Reverb Gain",      "Rev Gain", "",    { 0.01f, 0.5f, 0.0f, 0.5f }, 0.1f, 0.0f, dbConversionFunction);

    rebuildParams = { b.tuning, b.temperament, b.transpose, b.transposeUpper, b.transposeLower, b.transposePedal,
                      b.xtalkComp, b.xtalkTerm, b.xtalkWire, b.xtalkFloor, b.xtalkMin,
                      b.attackModel, b.clickLevel, b.clickMin, b.clickMax, b.releaseModel, b.releaseLevel,
                      b.scannerHz, b.scannerV1, b.scannerV2, b.scannerV3,
                      b.percFast, b.percSlow, b.percNorm, b.percSoft, b.percGain };

    organBuiltValues.resize (rebuildParams.size(), 0.0f);
    pendingBuiltValues.resize (rebuildParams.size(), 0.0f);
}

juce::StringPairArray OrganAudioProcessor::getOrganConfig() const
{
    auto& b = backside;

    juce::StringPairArray cfg;
    auto setF = [&] (const char* k, const gin::Parameter::Ptr& p) { cfg.set (k, juce::String (p->getUserValue(), 6)); };
    auto setI = [&] (const char* k, const gin::Parameter::Ptr& p) { cfg.set (k, juce::String (p->getUserValueInt())); };

    static const char* temperaments[] = { "gear60", "gear50", "equal" };
    static const char* models[]       = { "click", "shelf", "cosine", "linear" };

    // Rebuild group: consumed when the organ is initialized
    setF ("osc.tuning", b.tuning);
    cfg.set ("osc.temperament", temperaments[juce::jlimit (0, 2, b.temperament->getUserValueInt())]);
    setI ("midi.transpose", b.transpose);
    setI ("midi.upper.transpose", b.transposeUpper);
    setI ("midi.lower.transpose", b.transposeLower);
    setI ("midi.pedals.transpose", b.transposePedal);

    setF ("osc.compartment-crosstalk", b.xtalkComp);
    setF ("osc.terminalstrip-crosstalk", b.xtalkTerm);
    setF ("osc.wiring-crosstalk", b.xtalkWire);
    setF ("osc.contribution-floor", b.xtalkFloor);
    setF ("osc.contribution-min", b.xtalkMin);

    cfg.set ("osc.attack.model", models[juce::jlimit (0, 3, b.attackModel->getUserValueInt())]);
    setF ("osc.attack.click.level", b.clickLevel);
    setF ("osc.attack.click.minlength", b.clickMin);
    setF ("osc.attack.click.maxlength", b.clickMax);
    cfg.set ("osc.release.model", models[juce::jlimit (0, 3, b.releaseModel->getUserValueInt())]);
    setF ("osc.release.click.level", b.releaseLevel);

    setF ("scanner.hz", b.scannerHz);
    setF ("scanner.modulation.v1", b.scannerV1);
    setF ("scanner.modulation.v2", b.scannerV2);
    setF ("scanner.modulation.v3", b.scannerV3);

    setF ("osc.perc.fast", b.percFast);
    setF ("osc.perc.slow", b.percSlow);
    setF ("osc.perc.normal", b.percNorm);
    setF ("osc.perc.soft", b.percSoft);
    setF ("osc.perc.gain", b.percGain);

    // Live group: also passed at build time so a rebuilt organ starts correct
    setI ("whirl.bypass", b.lesBypass);
    setF ("whirl.horn.slowrpm", b.hornSlow);
    setF ("whirl.horn.fastrpm", b.hornFast);
    setF ("whirl.drum.slowrpm", b.drumSlow);
    setF ("whirl.drum.fastrpm", b.drumFast);
    setF ("whirl.horn.acceleration", b.hornAccel);
    setF ("whirl.horn.deceleration", b.hornDecel);
    setF ("whirl.drum.acceleration", b.drumAccel);
    setF ("whirl.drum.deceleration", b.drumDecel);
    setF ("whirl.horn.brakepos", b.hornBrake);
    setF ("whirl.drum.brakepos", b.drumBrake);
    setF ("whirl.horn.level", b.hornLevel);
    setF ("whirl.horn.leak", b.hornLeak);
    setF ("whirl.horn.width", b.hornWidth);
    setF ("whirl.drum.width", b.drumWidth);
    setF ("whirl.mic.distance", b.micDist);
    setF ("whirl.horn.mic.angle", b.micAngle);
    setF ("whirl.horn.radius", b.hornRadius);
    setF ("whirl.drum.radius", b.drumRadius);
    setF ("whirl.horn.offset.x", b.hornOffX);
    setF ("whirl.horn.offset.z", b.hornOffZ);

    setI ("whirl.horn.filter.a.type", b.hornF1Type);
    setF ("whirl.horn.filter.a.hz", b.hornF1Hz);
    setF ("whirl.horn.filter.a.q", b.hornF1Q);
    setF ("whirl.horn.filter.a.gain", b.hornF1Gain);
    setI ("whirl.horn.filter.b.type", b.hornF2Type);
    setF ("whirl.horn.filter.b.hz", b.hornF2Hz);
    setF ("whirl.horn.filter.b.q", b.hornF2Q);
    setF ("whirl.horn.filter.b.gain", b.hornF2Gain);
    setI ("whirl.drum.filter.type", b.drumFType);
    setF ("whirl.drum.filter.hz", b.drumFHz);
    setF ("whirl.drum.filter.q", b.drumFQ);
    setF ("whirl.drum.filter.gain", b.drumFGain);

    setF ("reverb.inputgain", b.revGain);

    return cfg;
}

bool OrganAudioProcessor::rebuildParamsChanged() const
{
    for (size_t i = 0; i < rebuildParams.size(); i++)
        if (! juce::approximatelyEqual (rebuildParams[i]->getUserValue(), organBuiltValues[i]))
            return true;

    return false;
}

void OrganAudioProcessor::handleAsyncUpdate()
{
    delete retiredOrgan.exchange (nullptr);

    if (rebuildRequested.load() && pendingOrgan.load() == nullptr)
    {
        for (size_t i = 0; i < rebuildParams.size(); i++)
            pendingBuiltValues[i] = rebuildParams[i]->getUserValue();

        auto newOrgan = new Organ (lastSampleRate, lastBlockSize, getOrganConfig());

        rebuildRequested.store (false);
        pendingOrgan.store (newOrgan);
    }
}

void OrganAudioProcessor::applyBacksideLive()
{
    auto& b = backside;

    organ->setLeslieBypass (b.lesBypass->getUserValueBool());
    organ->setLeslieSpeeds (b.hornSlow->getUserValue(), b.hornFast->getUserValue(),
                            b.drumSlow->getUserValue(), b.drumFast->getUserValue());
    organ->setLeslieDynamics (b.hornAccel->getUserValue(), b.hornDecel->getUserValue(),
                              b.drumAccel->getUserValue(), b.drumDecel->getUserValue());
    organ->setLeslieBrakes (b.hornBrake->getUserValue(), b.drumBrake->getUserValue());
    organ->setLeslieLevels (b.hornLevel->getUserValue(), b.hornLeak->getUserValue());
    organ->setLeslieWidths (b.hornWidth->getUserValue(), b.drumWidth->getUserValue());
    organ->setLeslieGeometry (b.micDist->getUserValue(), b.micAngle->getUserValue(),
                              b.hornRadius->getUserValue(), b.drumRadius->getUserValue(),
                              b.hornOffX->getUserValue(), b.hornOffZ->getUserValue());
    organ->setLeslieFilter (0, b.hornF1Type->getUserValueInt(), b.hornF1Hz->getUserValue(), b.hornF1Q->getUserValue(), b.hornF1Gain->getUserValue());
    organ->setLeslieFilter (1, b.hornF2Type->getUserValueInt(), b.hornF2Hz->getUserValue(), b.hornF2Q->getUserValue(), b.hornF2Gain->getUserValue());
    organ->setLeslieFilter (2, b.drumFType->getUserValueInt(), b.drumFHz->getUserValue(), b.drumFQ->getUserValue(), b.drumFGain->getUserValue());
    organ->setReverbInputGain (b.revGain->getUserValue());
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

    lastSampleRate = newSampleRate;
    lastBlockSize  = newSamplesPerBlock;

    // Audio isn't running yet, so any in-flight rebuild can be discarded
    cancelPendingUpdate();
    delete pendingOrgan.exchange (nullptr);
    delete retiredOrgan.exchange (nullptr);
    rebuildRequested.store (false);

    for (size_t i = 0; i < rebuildParams.size(); i++)
        organBuiltValues[i] = rebuildParams[i]->getUserValue();

    organ = std::make_unique<Organ> (newSampleRate, newSamplesPerBlock, getOrganConfig());
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

    // Swap in a rebuilt organ, or request a rebuild if a config param changed
    if (auto newOrgan = pendingOrgan.exchange (nullptr))
    {
        retiredOrgan.store (organ.release());
        organ.reset (newOrgan);
        std::copy (pendingBuiltValues.begin(), pendingBuiltValues.end(), organBuiltValues.begin());
        triggerAsyncUpdate();
    }
    else if (organ != nullptr && ! rebuildRequested.load() && rebuildParamsChanged())
    {
        rebuildRequested.store (true);
        triggerAsyncUpdate();
    }

    if (organ != nullptr)
    {
        applyBacksideLive();

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
