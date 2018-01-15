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
	MLOG("I'm called\n");
	auto seq = mpc->getSequencer().lock();
	bool wasPlaying = seq->isPlaying();
	if (wasPlaying) seq->stop();
	auto ams = mpc->getAudioMidiServices().lock();
	ams->destroyServices();
	ams->start("rtaudio", sampleRate);
	if (wasPlaying) seq->play();
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

void VmpcAudioProcessor::processMidiIn(MidiBuffer& midiMessages) {
	MidiBuffer::Iterator midiIterator(midiMessages);
	MidiMessage m;
	int midiEventPos;
	while (midiIterator.getNextEvent(m, midiEventPos)) {
		int frames = m.getTimeStamp();
		int velocity = m.getVelocity();

		if (m.isNoteOn()) {
			auto tootMsg = ctoot::midi::core::ShortMessage();
			auto data = std::vector<char>{ (char)ctoot::midi::core::ShortMessage::NOTE_ON, (char)(m.getNoteNumber()), (char)(velocity) };
			tootMsg.setMessage(data, 3);
			mpc->getMpcMidiInput(0)->transport(&tootMsg, 0);
		}
		else if (m.isNoteOff()) {
			auto tootMsg = ctoot::midi::core::ShortMessage();
			auto data = std::vector<char>{ (char)ctoot::midi::core::ShortMessage::NOTE_OFF, (char)(m.getNoteNumber()), (char)(velocity) };
			tootMsg.setMessage(data, 3);
			mpc->getMpcMidiInput(0)->transport(&tootMsg, 0);
		}
	}
}

void VmpcAudioProcessor::processMidiOut(MidiBuffer& midiMessages) {
	auto midiOutMsgQueues = mpc->getMidiPorts().lock()->getReceivers();

	for (auto& queue : *midiOutMsgQueues) {
		for (auto msg : queue) {
			auto velo = msg.getData2();
			if (velo == 0) continue;
			/*
			IMidiMsg imsg;
			imsg.MakeNoteOnMsg(msg.getData1(), velo, 0, msg.getChannel());
			imsg.mStatus = msg.getStatus();
			SendMidiMsg(&imsg);
			*/
		}
		for (auto msg : queue) {
			auto velo = msg.getData2();
			if (velo != 0) continue;
			/*
			IMidiMsg imsg;
			imsg.MakeNoteOffMsg(msg.getData1(), 0, msg.getChannel());
			imsg.mStatus = msg.getStatus();
			SendMidiMsg(&imsg);
			*/
		}
		queue.clear();
	}
}

void VmpcAudioProcessor::processTransport() {
	auto msGui = mpc->getUis().lock()->getMidiSyncGui();
	bool syncEnabled = msGui->getModeIn() == 1;

	if (syncEnabled) {
		/*
		const double tempo = GetTempo();
		if (tempo != m_Tempo || mpc->getSequencer().lock()->getTempo().toDouble() != tempo) {
		mpc->getSequencer().lock()->setTempo(BCMath(tempo));
		m_Tempo = tempo;
		}

		ITimeInfo ti;
		GetTime(&ti);

		bool isPlaying = ti.mTransportIsRunning;

		if (!m_WasPlaying && isPlaying)
		{
		mpc->getSequencer().lock()->playFromStart();
		}
		if (m_WasPlaying && !isPlaying) {
		mpc->getSequencer().lock()->stop();
		}
		m_WasPlaying = isPlaying;
		*/
	}
}

void VmpcAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	auto offlineServer = mpc->getAudioMidiServices().lock()->getOfflineServer();
	if (!offlineServer->isRealTime()) return;

	processMidiOut(midiMessages);
	processTransport();
	processMidiIn(midiMessages);

    ScopedNoDenormals noDenormals;
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

	auto server = mpc->getAudioMidiServices().lock()->getRtAudioServer();
	auto sr = getSampleRate();
/*		
	if (sr != 44100.0) {
		ipInL.setName("log");
		int numFramesToWork = ipInL.getFramesToWork(44100.0, sr, buffer.getNumSamples());

		//MLOG("\nnumFrames to work: " + std::to_string(numFramesToWork));
		std::vector<float> tempOutL(numFramesToWork);
		std::vector<float> tempOutR(numFramesToWork);
		std::vector<float*> tempOutLR{ &tempOutL[0], &tempOutR[0] };

		std::vector<float> tempInL(numFramesToWork);
		std::vector<float> tempInR(numFramesToWork);
		std::vector<float*> tempInLR = { &tempInL[0], &tempInR[0] };
		if (totalNumInputChannels >= 2) {
			auto chDataIn = buffer.getArrayOfReadPointers();
			ipInL.resample(chDataIn[0], buffer.getNumSamples(), sr, &tempInL[0], numFramesToWork, 44100.0);
			ipInR.resample(chDataIn[1], buffer.getNumSamples(), sr, &tempInR[0], numFramesToWork, 44100.0);
		}
		else {
			for (int i = 0; i < totalNumInputChannels; ++i)
				buffer.clear (i, 0, buffer.getNumSamples());
		}

		server->work(&tempInLR[0], &tempOutLR[0], numFramesToWork, totalNumInputChannels, totalNumOutputChannels);

		if (totalNumOutputChannels >= 2) {
			float* destOutL = buffer.getWritePointer(0);
			float* destOutR = buffer.getWritePointer(1);
			ipOutL.resample(&tempOutL, 44100.0, destOutL, buffer.getNumSamples(), sr);
			ipOutR.resample(&tempOutR, 44100.0, destOutR, buffer.getNumSamples(), sr);
		}
		else {
			for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
				buffer.clear(i, 0, buffer.getNumSamples());
		}
	}
	else {	
	*/
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

	//}
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
