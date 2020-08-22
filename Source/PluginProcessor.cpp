/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <audiomidi/AudioMidiServices.hpp>
#include <audiomidi/DiskRecorder.hpp>
#include <audiomidi/SoundRecorder.hpp>
#include <audiomidi/MpcMidiPorts.hpp>
#include <audiomidi/MpcMidiInput.hpp>

#include <Paths.hpp>
#include <sequencer/Sequencer.hpp>

#include <lcdgui/Background.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/screens/window/VmpcDirectToDiskRecorderScreen.hpp>
#include <lcdgui/screens/SyncScreen.hpp>

// ctoot
#include <audio/server/NonRealTimeAudioServer.hpp>
#include <midi/core/ShortMessage.hpp>

using namespace ctoot::midi::core;
using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;
using namespace mpc::lcdgui::screens::window;
using namespace std;

//==============================================================================
VmpcAudioProcessor::VmpcAudioProcessor()
     : AudioProcessor (BusesProperties()
		 .withInput  ("RECORD IN",  AudioChannelSet::stereo(), false)
		 .withOutput ("STEREO OUT", AudioChannelSet::stereo(), false)
		 .withOutput("MIX OUT 1/2", AudioChannelSet::stereo(), false)
		 .withOutput("MIX OUT 3/4", AudioChannelSet::stereo(), false)
		 .withOutput("MIX OUT 5/6", AudioChannelSet::stereo(), false)
		 .withOutput("MIX OUT 7/8", AudioChannelSet::stereo(), false))
{
	time_t currentTime = time(NULL);
	struct tm* currentLocalTime = localtime(&currentTime);
	auto timeString = string(asctime(currentLocalTime));

	moduru::Logger::l.setPath(mpc::Paths::logFilePath());
	moduru::Logger::l.log("\n\n-= vMPC2000XL v" + string(ProjectInfo::versionString) + " " + timeString.substr(0, timeString.length() - 1) + " =-\n");

	mpc.init(44100.f, 1, 5);
}

VmpcAudioProcessor::~VmpcAudioProcessor()
{
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
    auto seq = mpc.getSequencer().lock();
    bool wasPlaying = seq->isPlaying();
    
	if (wasPlaying) {
		seq->stop();
	}
    
	auto ams = mpc.getAudioMidiServices().lock();
	auto server = ams->getAudioServer();
    server->setSampleRate(sampleRate);
    server->resizeBuffers(samplesPerBlock);
	
	if (wasPlaying) {
		seq->play();
	}

	monoToStereoBuffer.clear();
	monoToStereoBuffer.setSize(2, samplesPerBlock);
}

void VmpcAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool VmpcAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
	return true;
	auto outs = layouts.outputBuses.size();
	if (layouts.inputBuses.size() > 1)
	{
		return false;
	}

	if (layouts.outputBuses.size() > 5)
	{
		return false;
	}

	// Mono input is anticipated, but outputs need to come in stereo pairs

	for (auto& bus : layouts.inputBuses)
	{
		if (bus != AudioChannelSet::mono() && bus != AudioChannelSet::stereo())
		{
			return false;
		}
	}

	for (auto& bus : layouts.outputBuses)
	{
		if (bus != AudioChannelSet::stereo())
		{
			return false;
		}
	}

    return true;
}

void VmpcAudioProcessor::processMidiIn(MidiBuffer& midiMessages) {
	MidiBuffer::Iterator midiIterator(midiMessages);
	juce::MidiMessage m;
	int midiEventPos;

	while (midiIterator.getNextEvent(m, midiEventPos))
	{
		int timeStamp = m.getTimeStamp();
		int velocity = m.getVelocity();

		if (m.isNoteOn())
		{
			m.getRawData();
			auto tootMsg = ShortMessage();
			tootMsg.setMessage(ShortMessage::NOTE_ON, m.getChannel() - 1, m.getNoteNumber(), velocity);
			mpc.getMpcMidiInput(0)->transport(&tootMsg, timeStamp);
		}
		else if (m.isNoteOff())
		{
			auto tootMsg = ShortMessage();
			tootMsg.setMessage(ShortMessage::NOTE_OFF, m.getChannel() - 1, m.getNoteNumber(), 0);
			mpc.getMpcMidiInput(0)->transport(&tootMsg, timeStamp);
		}
	}
}

void VmpcAudioProcessor::processMidiOut(MidiBuffer& midiMessages, int bufferSize) {
	auto midiOutMsgQueues = mpc.getMidiPorts().lock()->getReceivers();
	for (auto& queue : midiOutMsgQueues) {
		for (auto& msg : queue) {

			juce::uint8 velo = (juce::uint8) msg.getData2();
			if (velo == 0) continue;
			auto jmsg = juce::MidiMessage::noteOn(msg.getChannel() + 1, msg.getData1(), juce::uint8(velo));
			midiMessages.addEvent(jmsg, msg.bufferPos);
		}

		for (auto msg : queue) {
			auto velo = msg.getData2();
			if (velo != 0) continue;
			auto jmsg = juce::MidiMessage::noteOff(msg.getChannel() + 1, msg.getData1());
			midiMessages.addEvent(jmsg, msg.bufferPos);
		}
	}
	mpc.getMidiPorts().lock()->getReceivers()[0].clear();
	mpc.getMidiPorts().lock()->getReceivers()[1].clear();
}

void VmpcAudioProcessor::processTransport()
{
	if (JUCEApplication::isStandaloneApp())
	{
		return;
	}
	auto syncScreen = dynamic_pointer_cast<SyncScreen>(mpc.screens->getScreenComponent("sync"));

	bool syncEnabled = syncScreen->getModeIn() == 1;

	if (syncEnabled)
	{
		AudioPlayHead::CurrentPositionInfo info;
		getPlayHead()->getCurrentPosition(info);
		double tempo = info.bpm;
		
		if (tempo != m_Tempo || mpc.getSequencer().lock()->getTempo() != tempo)
		{
			mpc.getSequencer().lock()->setTempo(tempo);
			m_Tempo = tempo;
		}
		
		bool isPlaying = info.isPlaying;

		if (!wasPlaying && isPlaying)
		{
			mpc.getSequencer().lock()->playFromStart();
		}
		
		if (wasPlaying && !isPlaying) {
			mpc.getSequencer().lock()->stop();
		}
		wasPlaying = isPlaying;
	}
}

void VmpcAudioProcessor::checkBouncing()
{
	auto ams = mpc.getAudioMidiServices().lock();
	auto server = ams->getAudioServer();
	bool amsIsBouncing = ams->isBouncing();

	auto directToDiskRecorderScreen = dynamic_pointer_cast<VmpcDirectToDiskRecorderScreen>(mpc.screens->getScreenComponent("vmpc-direct-to-disk-recorder"));

	if (amsIsBouncing && !wasBouncing) {

		wasBouncing = true;
		
		if (directToDiskRecorderScreen->isOffline())
		{
			vector<int> rates{ 44100, 48000, 88200 };
			auto rate = rates[directToDiskRecorderScreen->getSampleRate()];
			ams->getFrameSequencer().lock()->start(rate);
			
			if (server->isRealTime())
			{
				server->setSampleRate(rate);
				server->setRealTime(false);
			}
		}
		else
		{
			ams->getFrameSequencer().lock()->start(getSampleRate());
		}

		for (auto& diskRecorder : ams->getDiskRecorders())
			diskRecorder.lock()->start();
	}
	else if (!amsIsBouncing && wasBouncing)
	{
		wasBouncing = false;

		if (directToDiskRecorderScreen->isOffline())
		{
			if (!server->isRealTime())
			{
				server->setSampleRate(getSampleRate());
				server->setRealTime(true);
			}
		}
	}
}

void VmpcAudioProcessor::checkSoundRecorder()
{
	auto ams = mpc.getAudioMidiServices().lock();
	auto recorder = ams->getSoundRecorder().lock();

	if (wasRecordingSound && !recorder->isRecording())
	{
		recorder->stop();
		ams->stopSoundRecorder();
	}

	if (!wasRecordingSound && ams->isRecordingSound())
	{
		wasRecordingSound = true;
		recorder->start();
	}
	else if (wasRecordingSound && !ams->isRecordingSound())
	{
		wasRecordingSound = false;
		recorder->stop();
	}
}

void VmpcAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	ScopedNoDenormals noDenormals;
\
	const int totalNumInputChannels = getTotalNumInputChannels();
	const int totalNumOutputChannels = getTotalNumOutputChannels();
	
	auto server = mpc.getAudioMidiServices().lock()->getAudioServer();

	if (!server->isRunning())
	{
		for (int i = 0; i < totalNumInputChannels; ++i)
		{
			buffer.clear(i, 0, buffer.getNumSamples());
		}
		return;
	}


	checkBouncing();
	checkSoundRecorder();

	if (!server->isRealTime())
	{
		for (int i = 0; i < totalNumInputChannels; ++i)
		{
			buffer.clear(i, 0, buffer.getNumSamples());
		}
		return;
	}

	processTransport();
	processMidiIn(midiMessages);
	processMidiOut(midiMessages, buffer.getNumSamples());

	auto chDataIn = buffer.getArrayOfReadPointers();
	auto chDataOut = buffer.getArrayOfWritePointers();

	if (totalNumInputChannels == 1)
	{
		monoToStereoBuffer.clear();
		monoToStereoBuffer.copyFrom(0, 0, buffer.getReadPointer(0), buffer.getNumSamples());
		monoToStereoBuffer.copyFrom(1, 0, buffer.getReadPointer(0), buffer.getNumSamples());
		server->work(monoToStereoBuffer.getArrayOfReadPointers(), chDataOut, buffer.getNumSamples(), 2, totalNumOutputChannels);
	}
	else
	{
		server->work(chDataIn, chDataOut, buffer.getNumSamples(), totalNumInputChannels, totalNumOutputChannels);
	}

	if (totalNumOutputChannels < 2)
	{
		for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		{
			buffer.clear(i, 0, buffer.getNumSamples());
		}
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
	auto editor = getActiveEditor();

	if (editor != nullptr)
	{
		auto w = editor->getWidth();
		auto h = editor->getHeight();
		std::unique_ptr<XmlElement> xml(new XmlElement("LastUIDimensions"));
		xml->setAttribute("w", w);
		xml->setAttribute("h", h);

		copyXmlToBinary(*xml, destData);
	}
}

void VmpcAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

	if (xmlState.get() != nullptr)
	{
		if (xmlState->hasTagName("LastUIDimensions"))
		{
			auto w = xmlState->getIntAttribute("w", 1298 / 2);
			auto h = xmlState->getIntAttribute("h", 994 / 2);
			lastUIWidth = w;
			lastUIHeight = h;
		}
	}
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VmpcAudioProcessor();
}
