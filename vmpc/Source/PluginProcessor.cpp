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
#include <audio/server/NonRealTimeAudioServer.hpp>
#include <audiomidi/MpcMidiPorts.hpp>
#include <audiomidi/MpcMidiInput.hpp>

#include <ui/midisync/MidiSyncGui.hpp>
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
	mpc->init("rtaudio", getSampleRate());
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
	if (JUCEApplication::isStandaloneApp()) {
		int latencySamples = deviceManager->getCurrentAudioDevice()->getInputLatencyInSamples() + deviceManager->getCurrentAudioDevice()->getOutputLatencyInSamples();
		MLOG("Total latency in samples reported by JUCE: " + to_string(latencySamples));
		auto midiOutput = deviceManager->getDefaultMidiOutput();
		if (midiOutput != nullptr) {
			midiOutput->stopBackgroundThread();
		}
	}
	auto seq = mpc->getSequencer().lock();
	bool wasPlaying = seq->isPlaying();
	if (wasPlaying) seq->stop();
	auto ams = mpc->getAudioMidiServices().lock();
	ams->destroyServices();
	ams->start("rtaudio", sampleRate);
	ams->setDisabled(false);
	ams->getRtAudioServer()->resizeBuffers(samplesPerBlock);
	if (wasPlaying) seq->play();
	if (JUCEApplication::isStandaloneApp()) {
		auto midiOutput = deviceManager->getDefaultMidiOutput();
		if (midiOutput != nullptr) {
			midiOutput->startBackgroundThread();
		}
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
			//MLOG("\nNote on timestamp: " + std::to_string(timeStamp));
			m.getRawData();
			auto tootMsg = ctoot::midi::core::ShortMessage();
			tootMsg.setMessage(ctoot::midi::core::ShortMessage::NOTE_ON, m.getChannel() - 1, m.getNoteNumber(), velocity);
			//auto data = std::vector<char>{ (char)ctoot::midi::core::ShortMessage::NOTE_ON, (char)(m.getNoteNumber()), (char)(velocity) };
			//tootMsg.setMessage(data, 3);
			mpc->getMpcMidiInput(0)->transport(&tootMsg, timeStamp);
		}
		else if (m.isNoteOff()) {
			auto tootMsg = ctoot::midi::core::ShortMessage();
			//auto data = std::vector<char>{ (char)ctoot::midi::core::ShortMessage::NOTE_OFF, (char)(m.getNoteNumber()), (char)(velocity) };
			//tootMsg.setMessage(data, 3);
			tootMsg.setMessage(ctoot::midi::core::ShortMessage::NOTE_OFF, m.getChannel() - 1, m.getNoteNumber(), 0);
			mpc->getMpcMidiInput(0)->transport(&tootMsg, timeStamp);
		}
	}
}

void VmpcAudioProcessor::processMidiOut(MidiBuffer& midiMessages, int bufferSize) {
	bool standalone = JUCEApplication::isStandaloneApp();
	MidiOutput* midiOutput = nullptr;
	if (standalone) {
		midiOutput = deviceManager->getDefaultMidiOutput();
	}

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
	if (standalone) {
		if (midiOutput != nullptr) {
			int latencySamples = deviceManager->getCurrentAudioDevice()->getInputLatencyInSamples() + deviceManager->getCurrentAudioDevice()->getOutputLatencyInSamples();
			//double latencyMs = (double) (bufferSize) / (getSampleRate() / 1000.0);
			double latencyMs = (double)(latencySamples) / (getSampleRate() / 1000.0);
			midiOutput->sendBlockOfMessages(midiMessages, Time::getMillisecondCounter() + latencyMs, getSampleRate());
		}
		midiMessages.clear(); // necessary?
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

		if (!m_WasPlaying && isPlaying)
		{
			mpc->getSequencer().lock()->playFromStart();
		}
		
		if (m_WasPlaying && !isPlaying) {
			mpc->getSequencer().lock()->stop();
		}
		m_WasPlaying = isPlaying;
	}
}

void VmpcAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	ScopedNoDenormals noDenormals;
\
	const int totalNumInputChannels = getTotalNumInputChannels();
	const int totalNumOutputChannels = getTotalNumOutputChannels();
	
	if (mpc->getAudioMidiServices().lock()->isDisabled()) {
		for (int i = 0; i < totalNumInputChannels; ++i)
			buffer.clear(i, 0, buffer.getNumSamples());
		return;
	}
	auto offlineServer = mpc->getAudioMidiServices().lock()->getOfflineServer();
	if (!offlineServer->isRealTime()) {
		for (int i = 0; i < totalNumInputChannels; ++i)
			buffer.clear(i, 0, buffer.getNumSamples());
		return;
	}

	processTransport();
	processMidiIn(midiMessages);
	//processMidiOut(midiMessages, buffer.getNumSamples() * (totalNumInputChannels + totalNumOutputChannels) * 0.5);
	processMidiOut(midiMessages, buffer.getNumSamples());

	auto server = mpc->getAudioMidiServices().lock()->getRtAudioServer();
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
