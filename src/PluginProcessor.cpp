/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DistortionPluginAudioProcessor::DistortionPluginAudioProcessor()
     : apvts(*this, nullptr, "PARAMETERS",
        {
             std::make_unique<juce::AudioParameterFloat>("distortion", "DISTORTION", 0.0f, 1.0f, 0.5f),
             std::make_unique<juce::AudioParameterFloat>("level", "LEVEL", 0.0f, 1.0f, 1.0f),
             std::make_unique<juce::AudioParameterBool>("enable", "ENABLE", true)
        }),
    distortion(distortionValue),
    clipping(levelValue)
{
}

DistortionPluginAudioProcessor::~DistortionPluginAudioProcessor()
{
}

//==============================================================================
const juce::String DistortionPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DistortionPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DistortionPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DistortionPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DistortionPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DistortionPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DistortionPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DistortionPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String DistortionPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void DistortionPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void DistortionPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    distortion.prepare(sampleRate);
    clipping.prepare(sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = 2;
    iir.state = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 5000.0);
    iir.prepare(spec);
}

void DistortionPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool DistortionPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void DistortionPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

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


    distortionValue = *apvts.getRawParameterValue("distortion");
    levelValue = *apvts.getRawParameterValue("level");
    effectOn = apvts.getParameterAsValue("enable").getValue();

    distortion.setKnob(distortionValue);
    clipping.setKnob(levelValue);


    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            float x = buffer.getReadPointer(channel)[n];
            if (effectOn)
            {
                float ds = distortion.processSample(x);
                //float o = tone.processSample(dis);
                buffer.getWritePointer(channel)[n] = clipping.processSample(ds);
            }
            else
            {
                buffer.getWritePointer(channel)[n] = x;
            }
        }
    }

    // clean those high frequences
    juce::dsp::AudioBlock<float> block(buffer);
    iir.process(juce::dsp::ProcessContextReplacing<float>(block));
}

//==============================================================================
bool DistortionPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DistortionPluginAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void DistortionPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void DistortionPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DistortionPluginAudioProcessor();
}
