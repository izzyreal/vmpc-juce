#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "version.h"

#include <audiomidi/AudioMidiServices.hpp>
#include <audiomidi/DiskRecorder.hpp>
#include <audiomidi/SoundRecorder.hpp>
#include <audiomidi/MpcMidiPorts.hpp>
#include <audiomidi/MpcMidiInput.hpp>

#include <file/aps/ApsParser.hpp>
#include <disk/ApsLoader.hpp>
#include <disk/AbstractDisk.hpp>

#include <Paths.hpp>
#include <sequencer/Sequencer.hpp>

#include <lcdgui/Background.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/screens/window/VmpcDirectToDiskRecorderScreen.hpp>
#include <lcdgui/screens/SyncScreen.hpp>

// ctoot
#include <audio/server/NonRealTimeAudioServer.hpp>
#include <midi/core/ShortMessage.hpp>

// moduru
#include <lang/StrUtil.hpp>
#include <file/FileUtil.hpp>

using namespace ctoot::midi::core;
using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;
using namespace mpc::lcdgui::screens::window;
using namespace mpc::file::aps;
using namespace mpc::disk;
using namespace moduru::lang;
using namespace moduru::file;
using namespace std;

VmpcAudioProcessor::VmpcAudioProcessor()
     : AudioProcessor (BusesProperties()
         .withInput  ("RECORD IN",  AudioChannelSet::stereo(), true)
         .withOutput ("STEREO OUT", AudioChannelSet::stereo(), true)
         .withOutput("MIX OUT 1/2", AudioChannelSet::stereo(), false)
         .withOutput("MIX OUT 3/4", AudioChannelSet::stereo(), false)
         .withOutput("MIX OUT 5/6", AudioChannelSet::stereo(), false)
         .withOutput("MIX OUT 7/8", AudioChannelSet::stereo(), false)
)
{
    time_t currentTime = time(nullptr);
    struct tm* currentLocalTime = localtime(&currentTime);
    auto timeString = string(asctime(currentLocalTime));

    moduru::Logger::l.setPath(mpc::Paths::logFilePath());
    moduru::Logger::l.log("\n\n-= vMPC2000XL v" + string(version::get()) + " " + timeString.substr(0, timeString.length() - 1) + " =-\n");

    mpc.init(44100.f, 1, 5);
}

VmpcAudioProcessor::~VmpcAudioProcessor()
{
}

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
    bool seqIsPlaying = seq->isPlaying();
    
    if (seqIsPlaying)
        seq->stop();
    
    auto ams = mpc.getAudioMidiServices().lock();
    auto server = ams->getAudioServer();
    server->setSampleRate(static_cast<int>(sampleRate));
    server->resizeBuffers(samplesPerBlock);
    
    if (seqIsPlaying)
        seq->play();

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
    /*
    auto outs = layouts.outputBuses.size();
    if (layouts.inputBuses.size() > 1)
        return false;

    if (layouts.outputBuses.size() > 5)
        return false;

    // Mono input is anticipated, but outputs need to come in stereo pairs

    for (auto& bus : layouts.inputBuses)
    {
        if (bus != AudioChannelSet::mono() && bus != AudioChannelSet::stereo())
            return false;
    }

    for (auto& bus : layouts.outputBuses)
    {
        if (bus != AudioChannelSet::stereo())
            return false;
    }

    return true;
    */
}

void VmpcAudioProcessor::processMidiIn(MidiBuffer& midiMessages) {
    
    for (const auto meta : midiMessages)
    {
        const auto m = meta.getMessage();
        int timeStamp = static_cast<int>(m.getTimeStamp());
        int velocity = m.getVelocity();
        
        if (m.isNoteOn())
        {
            m.getRawData();
            ShortMessage tootMsg;
            tootMsg.setMessage(ShortMessage::NOTE_ON, m.getChannel() - 1, m.getNoteNumber(), velocity);
            mpc.getMpcMidiInput(0)->transport(&tootMsg, timeStamp);
        }
        else if (m.isNoteOff())
        {
            ShortMessage tootMsg;
            tootMsg.setMessage(ShortMessage::NOTE_OFF, m.getChannel() - 1, m.getNoteNumber(), 0);
            mpc.getMpcMidiInput(0)->transport(&tootMsg, timeStamp);
        }
        else if (m.isController())
        {
            ShortMessage tootMsg;
            tootMsg.setMessage(ShortMessage::CONTROL_CHANGE, m.getChannel() - 1, m.getControllerNumber(), m.getControllerValue());
            mpc.getMpcMidiInput(0)->transport(&tootMsg, timeStamp);
        }
    }
}

void VmpcAudioProcessor::processMidiOut(MidiBuffer& midiMessages)
{
    auto midiOutMsgQueues = mpc.getMidiPorts().lock()->getReceivers();
    for (auto& queue : midiOutMsgQueues)
    {
        for (auto& msg : queue)
        {
            juce::uint8 velo = (juce::uint8) msg.getData2();
            if (velo == 0) continue;
            auto jmsg = juce::MidiMessage::noteOn(msg.getChannel() + 1, msg.getData1(), juce::uint8(velo));
            midiMessages.addEvent(jmsg, msg.bufferPos);
        }

        for (auto msg : queue)
        {
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
        else if (directToDiskRecorderScreen->getRecord() != 4)
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
            buffer.clear(i, 0, buffer.getNumSamples());

        return;
    }

    processTransport();
    processMidiIn(midiMessages);
    processMidiOut(midiMessages);

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
            buffer.clear(i, 0, buffer.getNumSamples());
    }
}

bool VmpcAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* VmpcAudioProcessor::createEditor()
{
    mpc.getLayeredScreen().lock()->setDirty();
    return new VmpcAudioProcessorEditor (*this);
}

void VmpcAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    MLOG("getStateInformation");
    auto editor = getActiveEditor();

    auto root = new XmlElement("root");
    unique_ptr<XmlElement> xml(root);

    auto juce_ui = new XmlElement("JUCE-UI");
    root->addChildElement(juce_ui);

    if (editor != nullptr)
    {
        auto w = editor->getWidth();
        auto h = editor->getHeight();
        juce_ui->setAttribute("w", w);
        juce_ui->setAttribute("h", h);
    }
    
    auto layeredScreen = mpc.getLayeredScreen().lock();
    
    auto screen = layeredScreen->getCurrentScreenName();
    auto focus = mpc.getLayeredScreen().lock()->getFocus();
    auto soundIndex = mpc.getSampler().lock()->getSoundIndex();
    
    ApsParser apsParser(mpc, "stateinfo");

    auto mpc_ui = new XmlElement("MPC-UI");
    mpc_ui->setAttribute("screen", screen);
    mpc_ui->setAttribute("focus", focus);
    mpc_ui->setAttribute("soundIndex", soundIndex);
    mpc_ui->setAttribute("currentDir", mpc.getDisk().lock()->getAbsolutePath());
    
    auto mpc_aps = new XmlElement("APS");
    auto apsBytes = apsParser.getBytes();
    
    MemoryOutputStream encoded;
    Base64::convertToBase64(encoded, &apsBytes[0], apsBytes.size());
    MLOG("apsBytes size: " + to_string(apsBytes.size()));
    mpc_aps->setAttribute("aps", encoded.toString());
    mpc_aps->setAttribute("size", (int) apsBytes.size());
    
    root->addChildElement(mpc_ui);
    root->addChildElement(mpc_aps);
    copyXmlToBinary(*xml, destData);
}

void VmpcAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    MLOG("setStateInformation");
    unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        XmlElement* element = xmlState->getFirstChildElement();
        do
        {
            MLOG(element->getTagName().toStdString());
            if (element->getTagName().compare("JUCE-UI") == 0)
            {
                auto w = element->getIntAttribute("w", 1298 / 2);
                auto h = element->getIntAttribute("h", 994 / 2);
                lastUIWidth = w;
                lastUIHeight = h;
            }
            else if (element->getTagName().compare("MPC-UI") == 0)
            {
                auto currentDir = element->getStringAttribute("currentDir").toStdString();
                
                // Be careful, hardcoded store. For now ok, but if stores become user configurable this needs
                // to be dynamic.
                auto storesPath = mpc::Paths::storesPath() + "MPC2000XL";
                auto resPathIndex = currentDir.find(storesPath);
                
                if (resPathIndex != string::npos)
                {
                    auto trimmedCurrentDir = currentDir.substr(resPathIndex + storesPath.length());
                    MLOG("currentDir: " + currentDir);
                    MLOG("trimmedCurrentDir: " + trimmedCurrentDir);
                    auto splitTrimmedDir = StrUtil::split(trimmedCurrentDir, FileUtil::getSeparator()[0]);
                    
                    for (auto& s : splitTrimmedDir)
                    {
                        MLOG("split: " + s);
                        mpc.getDisk().lock()->moveForward(s);
                        mpc.getDisk().lock()->initFiles();
                    }
                }
                
                mpc.getLayeredScreen().lock()->openScreen(element->getStringAttribute("screen").toStdString());
                mpc.getLayeredScreen().lock()->setFocus(element->getStringAttribute("focus").toStdString());
                mpc.getSampler().lock()->setSoundIndex(element->getIntAttribute("soundIndex"));
            }
            else if (element->getTagName().compare("APS") == 0)
            {
                MemoryOutputStream decoded;
                Base64::convertFromBase64(decoded, element->getStringAttribute("aps"));
                auto decodedData = (char*) (decoded.getData());
                
                vector<char> asCharVector(decodedData, decodedData + element->getIntAttribute("size"));
                
                MLOG("asCharVector size: " + to_string(asCharVector.size()));
                
                if (asCharVector.size() != 0)
                {
                    ApsParser apsParser(mpc, asCharVector, "auto-state-from-xml");
                    ApsLoader::loadFromParsedAps(apsParser, mpc, true);
                }
            }
        }
        while ( (element = element->getNextElement()) != nullptr);
    }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VmpcAudioProcessor();
}
