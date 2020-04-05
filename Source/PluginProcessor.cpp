/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <audiomidi/AudioMidiServices.hpp>
#include <audiomidi/ExportAudioProcessAdapter.hpp>

#include <audio/server/ExternalAudioServer.hpp>
#include <audio/server/NonRealTimeAudioServer.hpp>
#include <audiomidi/MpcMidiPorts.hpp>
#include <audiomidi/MpcMidiInput.hpp>

#include <ui/midisync/MidiSyncGui.hpp>
#include <ui/vmpc/DirectToDiskRecorderGui.hpp>
#include <sequencer/Sequencer.hpp>

// ctoot
#include <midi/core/ShortMessage.hpp>

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
	mpc->init(44100.f, 1, 5);
	mpc->getLayeredScreen().lock()->openScreen("sequencer");
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
    auto seq = mpc->getSequencer().lock();
    bool wasPlaying = seq->isPlaying();
    
	if (wasPlaying) {
		seq->stop();
	}
    
	auto ams = mpc->getAudioMidiServices().lock();
	auto server = ams->getExternalAudioServer();
    server->setSampleRate(sampleRate);
    server->resizeBuffers(samplesPerBlock);
	
	if (wasPlaying) {
		seq->play();
	}
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
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

void VmpcAudioProcessor::processMidiIn(MidiBuffer& midiMessages) {
	MidiBuffer::Iterator midiIterator(midiMessages);
	MidiMessage m;
	int midiEventPos;
	while (midiIterator.getNextEvent(m, midiEventPos)) {
		int timeStamp = m.getTimeStamp();
		int velocity = m.getVelocity();

		if (m.isNoteOn()) {
			m.getRawData();
			auto tootMsg = ctoot::midi::core::ShortMessage();
			tootMsg.setMessage(ctoot::midi::core::ShortMessage::NOTE_ON, m.getChannel() - 1, m.getNoteNumber(), velocity);
			mpc->getMpcMidiInput(0)->transport(&tootMsg, timeStamp);
		}
		else if (m.isNoteOff()) {
			auto tootMsg = ctoot::midi::core::ShortMessage();
			tootMsg.setMessage(ctoot::midi::core::ShortMessage::NOTE_OFF, m.getChannel() - 1, m.getNoteNumber(), 0);
			mpc->getMpcMidiInput(0)->transport(&tootMsg, timeStamp);
		}
	}
}

void VmpcAudioProcessor::processMidiOut(MidiBuffer& midiMessages, int bufferSize) {
	auto midiOutMsgQueues = mpc->getMidiPorts().lock()->getReceivers();
	for (auto& queue : *midiOutMsgQueues) {
		for (auto msg : queue) {

			juce::uint8 velo = (juce::uint8) msg.getData2();
			if (velo == 0) continue;
			auto jmsg = MidiMessage::noteOn(msg.getChannel() + 1, msg.getData1(), juce::uint8(velo));
			midiMessages.addEvent(jmsg, msg.bufferPos);
		}
		for (auto msg : queue) {
			auto velo = msg.getData2();
			if (velo != 0) continue;
			auto jmsg = MidiMessage::noteOff(msg.getChannel() + 1, msg.getData1());
			midiMessages.addEvent(jmsg, msg.bufferPos);
		}
		queue.clear();
	}
}

void VmpcAudioProcessor::processTransport() {
	if (JUCEApplication::isStandaloneApp()) return;
	auto msGui = mpc->getUis().lock()->getMidiSyncGui();
	bool syncEnabled = msGui->getModeIn() == 1;

	if (syncEnabled) {
		AudioPlayHead::CurrentPositionInfo info;
		getPlayHead()->getCurrentPosition(info);
		double tempo = info.bpm;
		if (tempo != m_Tempo || mpc->getSequencer().lock()->getTempo().toDouble() != tempo) {
			mpc->getSequencer().lock()->setTempo(BCMath(tempo));
			m_Tempo = tempo;
		}
		
		bool isPlaying = info.isPlaying;

		if (!wasPlaying && isPlaying)
		{
			mpc->getSequencer().lock()->playFromStart();
		}
		
		if (wasPlaying && !isPlaying) {
			mpc->getSequencer().lock()->stop();
		}
		wasPlaying = isPlaying;
	}
}

void VmpcAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	ScopedNoDenormals noDenormals;
\
	const int totalNumInputChannels = getTotalNumInputChannels();
	const int totalNumOutputChannels = getTotalNumOutputChannels();
	
	auto ams = mpc->getAudioMidiServices().lock();
	auto server = ams->getExternalAudioServer();
	auto offlineServer = ams->getOfflineServer();

	if (!offlineServer->isRunning()) {
		for (int i = 0; i < totalNumInputChannels; ++i)
			buffer.clear(i, 0, buffer.getNumSamples());
		return;
	}
	bool amsIsBouncing = ams->isBouncing();

	if (amsIsBouncing && !wasBouncing) {
		MLOG("JUCE will start bouncing now...")

		wasBouncing = true;

		auto directToDiskRecorderGui = mpc->getUis().lock()->getD2DRecorderGui();
		if (directToDiskRecorderGui->isOffline()) {
			if (offlineServer->isRealTime()) {
				vector<int> rates{ 44100, 48000, 88200 };
				server->setSampleRate(rates[directToDiskRecorderGui->getSampleRate()]);
				offlineServer->setRealTime(false);
			}
		}

		for (auto& eapa : ams->exportProcesses) {
			eapa->start();
		}

	} else if (!amsIsBouncing && wasBouncing) {
		MLOG("JUCE will stop bouncing now...")
		wasBouncing = false;

		auto directToDiskRecorderGui = mpc->getUis().lock()->getD2DRecorderGui();
		if (directToDiskRecorderGui->isOffline()) {
			if (!offlineServer->isRealTime()) {
				server->setSampleRate(getSampleRate());
				offlineServer->setRealTime(true);
			}
		}
	}

	if (!offlineServer->isRealTime()) {
		for (int i = 0; i < totalNumInputChannels; ++i)
			buffer.clear(i, 0, buffer.getNumSamples());
		return;
	}

	processTransport();
	processMidiIn(midiMessages);
	processMidiOut(midiMessages, buffer.getNumSamples());

	auto chDataIn = buffer.getArrayOfReadPointers();
	auto chDataOut = buffer.getArrayOfWritePointers();

	if (totalNumInputChannels < 2) {
		for (int i = 0; i < totalNumInputChannels; ++i)
			buffer.clear(i, 0, buffer.getNumSamples());
	}

	server->work(chDataIn, chDataOut, buffer.getNumSamples(), totalNumInputChannels, totalNumOutputChannels);

	if (totalNumOutputChannels < 2) {
		for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
			buffer.clear(i, 0, buffer.getNumSamples());
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
