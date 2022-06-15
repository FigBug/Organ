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
    for (int i = 0; i < 9; i++)
    {
        auto num = juce::String (i + 1);
        pedalDrawBars[i] = addExtParam ("pedal" + num, "Pedal Draw Bar " + num, "Pedal " + num, "", { 0.0f, 8.0f, 1.0f, 1.0f}, defaultPreset[i], 0.0f);
    }
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
    
    if (organ != nullptr)
    {
        for (int i = 0; i < 9; i++) organ->setUpperDrawBar (i, upperDrawBars[i]->getUserValueInt());
        for (int i = 0; i < 9; i++) organ->setLowerDrawBar (i, upperDrawBars[i]->getUserValueInt());
        for (int i = 0; i < 9; i++) organ->setPedalDrawBar (i, upperDrawBars[i]->getUserValueInt());

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
