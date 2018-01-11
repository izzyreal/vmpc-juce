/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <audiomidi/AudioMidiServices.hpp>
#include <audio/server/RtAudioServer.hpp>

//==============================================================================
VmpcAudioProcessor::VmpcAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	mpc = new mpc::Mpc();
	mpc->init("rtaudio");
}

VmpcAudioProcessor::~VmpcAudioProcessor()
{
	delete mpc;
}

//==============================================================================
const String VmpcAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VmpcAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VmpcAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VmpcAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VmpcAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VmpcAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VmpcAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VmpcAudioProcessor::setCurrentProgram (int index)
{
}

const String VmpcAudioProcessor::getProgramName (int index)
{
    return {};
}

void VmpcAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void VmpcAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void VmpcAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VmpcAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

void VmpcAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	auto server = mpc->getAudioMidiServices().lock()->getRtAudioServer();
	auto sr = getSampleRate();

	
	std::vector<float> tempInL;
	std::vector<float> tempInR;
	std::vector<float*> tempInLR;

	const float** channelDataIn = 0;
	if (totalNumInputChannels != 0) {
		channelDataIn = buffer.getArrayOfReadPointers();
	}
	float** channelDataOut = 0;
	if (totalNumOutputChannels != 0) {
		channelDataOut = buffer.getArrayOfWritePointers();
	}
	
	if (sr != 44100.0) {
		int numFramesToWork = ipOutL.getFramesToWork(44100.0, sr, buffer.getNumSamples());

		std::vector<float> tempOutL(numFramesToWork);
		std::vector<float> tempOutR(numFramesToWork);
		std::vector<float*> tempOutLR{ &tempOutL[0], &tempOutR[0] };

		if (totalNumInputChannels >= 2) {
			for (int i = 0; i < numFramesToWork; i++) {
				tempInL.push_back(channelDataIn[0][i]);
				tempInR.push_back(channelDataIn[1][i]);
			}
			tempInLR = { &tempInL[0], &tempInR[0] };
		}

		server->work(&tempInLR[0], &tempOutLR[0], numFramesToWork, totalNumInputChannels, totalNumOutputChannels);

		if (totalNumOutputChannels >= 2) {
			float* destOutL = buffer.getWritePointer(0);
			float* destOutR = buffer.getWritePointer(1);
			ipOutL.resample(&tempOutL, 44100.0, destOutL, buffer.getNumSamples(), sr);
			ipOutR.resample(&tempOutR, 44100.0, destOutR, buffer.getNumSamples(), sr);
		}

	}
	else {	
		if (totalNumInputChannels >= 2) {
			for (int i = 0; i < buffer.getNumSamples(); i++) {
				tempInL.push_back(channelDataIn[0][i]);
				tempInR.push_back(channelDataIn[1][i]);
			}
			tempInLR = { &tempInL[0], &tempInR[0] };
		}
		server->work(&tempInLR[0], channelDataOut, buffer.getNumSamples(), totalNumInputChannels, totalNumOutputChannels);
	}
}

//==============================================================================
bool VmpcAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* VmpcAudioProcessor::createEditor()
{
    return new VmpcAudioProcessorEditor (*this);
}

//==============================================================================
void VmpcAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void VmpcAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VmpcAudioProcessor();
}
