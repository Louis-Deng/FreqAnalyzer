/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FreqAnalyzerInDualMixerAudioProcessor::FreqAnalyzerInDualMixerAudioProcessor()
: mBufferSize(0)
, mSampleRate(0.0)
, parameters (*this, nullptr, juce::Identifier ("PVT"),{
    
})
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // initialzing all the unique_ptr_s
    for (int i=0;i<2;i++)
    {
        mDWM[i].reset( new DWmixer<float> );
    }
}

FreqAnalyzerInDualMixerAudioProcessor::~FreqAnalyzerInDualMixerAudioProcessor()
{
}

//==============================================================================
const juce::String FreqAnalyzerInDualMixerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FreqAnalyzerInDualMixerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FreqAnalyzerInDualMixerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FreqAnalyzerInDualMixerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FreqAnalyzerInDualMixerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FreqAnalyzerInDualMixerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FreqAnalyzerInDualMixerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FreqAnalyzerInDualMixerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FreqAnalyzerInDualMixerAudioProcessor::getProgramName (int index)
{
    return {};
}

void FreqAnalyzerInDualMixerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FreqAnalyzerInDualMixerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void FreqAnalyzerInDualMixerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FreqAnalyzerInDualMixerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FreqAnalyzerInDualMixerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    
    // copy dry input all channels
    juce::AudioBuffer<float> drySamples;
    drySamples.makeCopyOf(buffer);
    
    // for each output channel
    for (int channel = 0; channel < totalNumOutputChannels; channel++)
    {
        int dryReadFromChan = 0;
        if (totalNumInputChannels == 1 && totalNumOutputChannels == 1)
        {
            // mono -> mono
            dryReadFromChan = 0;
        }
        else if (totalNumInputChannels == 1 && totalNumOutputChannels == 2)
        {
            // mono -> stereo
            dryReadFromChan = 0;
            // copy inL to inR in buffer
            buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
        }
        else
        {
            // stereo -> stereo
            dryReadFromChan = channel;
        }
        
        auto* drySamplesPtr = drySamples.getReadPointer(dryReadFromChan);
        auto* channelDSP = buffer.getWritePointer(channel);
        
        // dry wet mixer
        mDWM[channel]->processBuffer(drySamplesPtr,channelDSP,buffer.getNumSamples());
    }
}

//==============================================================================
bool FreqAnalyzerInDualMixerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FreqAnalyzerInDualMixerAudioProcessor::createEditor()
{
    return new FreqAnalyzerInDualMixerAudioProcessorEditor (*this);
}

//==============================================================================
void FreqAnalyzerInDualMixerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
    //DBG("SAVED STATE INFO IN" << "..." << "! ");
}

void FreqAnalyzerInDualMixerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
     
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
            //DBG("GOT STATE INFO FROM " << "..." << "! ");
        }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FreqAnalyzerInDualMixerAudioProcessor();
}
