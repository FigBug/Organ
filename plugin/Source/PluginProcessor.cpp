#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace gin;

//==============================================================================
OrganAudioProcessor::OrganAudioProcessor()
{
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
        organ->processBlock (buffer, midi);
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
