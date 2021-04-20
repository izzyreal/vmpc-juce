#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "version.h"

#include <audiomidi/AudioMidiServices.hpp>
#include <audiomidi/DiskRecorder.hpp>
#include <audiomidi/SoundRecorder.hpp>
#include <audiomidi/MpcMidiPorts.hpp>
#include <audiomidi/MpcMidiInput.hpp>

#include <file/aps/ApsParser.hpp>
#include <file/all/AllParser.hpp>
#include <file/sndwriter/SndWriter.hpp>
#include <file/sndreader/SndReader.hpp>
#include <disk/ApsLoader.hpp>
#include <disk/AllLoader.hpp>
#include <disk/AbstractDisk.hpp>

#include <Paths.hpp>
#include <sequencer/Sequencer.hpp>

#include <lcdgui/Background.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/screens/VmpcAutoSaveScreen.hpp>
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
using namespace mpc::file::all;
using namespace mpc::file::sndwriter;
using namespace mpc::file::sndreader;
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
    moduru::Logger::l.log("\n\n-= VMPC2000XL v" + string(version::get()) + " " + timeString.substr(0, timeString.length() - 1) + " =-\n");
    
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
    auto syncScreen = mpc.screens->get<SyncScreen>("sync");
    
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
    
    auto directToDiskRecorderScreen = mpc.screens->get<VmpcDirectToDiskRecorderScreen>("vmpc-direct-to-disk-recorder");
    
    if (amsIsBouncing && !wasBouncing) {
        
        wasBouncing = true;
        
        if (directToDiskRecorderScreen->isOffline())
        {
            vector<int> rates{ 44100, 48000, 88200 };
            auto rate = rates[static_cast<size_t>(directToDiskRecorderScreen->getSampleRate())];
            ams->getFrameSequencer().lock()->start(rate);
            
            if (server->isRealTime())
            {
                server->setSampleRate(rate);
                server->setRealTime(false);
            }
        }
        else if (directToDiskRecorderScreen->getRecord() != 4)
        {
            ams->getFrameSequencer().lock()->start(static_cast<int>(getSampleRate()));
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
                server->setSampleRate(static_cast<int>(getSampleRate()));
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
 
    auto vmpcAutoSaveScreen = mpc.screens->get<VmpcAutoSaveScreen>("vmpc-auto-save");
    
    if (wrapperType == wrapperType_Standalone)
    {
        if (vmpcAutoSaveScreen->getAutoSaveOnExit() == 0)
        {
            // For now we will assume any call in standalone mode to getStateInformation
            // is when VMPC2000XL exits.
            return;
        }
        else if (vmpcAutoSaveScreen->getAutoSaveOnExit() == 1)
        {
            // The user wants to be asked.
            auto result = AlertWindow::showOkCancelBox (
                                                        AlertWindow::InfoIcon,
                                                        "Auto-save this VMPC2000XL session?",
                                                        "This will allow you to continue your work next time you start VMPC2000XL",
                                                        "Don't save",
                                                        "Save");
            if (result)
            {
                // MLOG("Not saving current session");
                // Our work here is done
                return;
            }
            else
            {
                // MLOG("Auto-saving session");
                // We may continue the below routine.
            }
        }
    }
    
    auto layeredScreen = mpc.getLayeredScreen().lock();
    
    auto screen = layeredScreen->getCurrentScreenName();
    auto focus = mpc.getLayeredScreen().lock()->getFocus();
    auto soundIndex = mpc.getSampler().lock()->getSoundIndex();
    
    auto mpc_ui = new XmlElement("MPC-UI");
    root->addChildElement(mpc_ui);
    
    mpc_ui->setAttribute("screen", screen);
    mpc_ui->setAttribute("focus", focus);
    mpc_ui->setAttribute("soundIndex", soundIndex);
    mpc_ui->setAttribute("currentDir", mpc.getDisk().lock()->getAbsolutePath());
    
    ApsParser apsParser(mpc, "stateinfo");
    auto apsBytes = apsParser.getBytes();
    auto sounds = mpc.getSampler().lock()->getSounds();
    
    for (size_t i = 0; i < sounds.size(); i++)
    {
        auto elementName = "sound" + to_string(i);
        auto soundElement = new XmlElement(elementName.c_str());
        root->addChildElement(soundElement);
        
        auto sound = sounds[i].lock();
        SndWriter sndWriter(sound.get());
        auto data = sndWriter.getSndFileArray();
        
        MemoryOutputStream encodedSound;
        Base64::convertToBase64(encodedSound, &data[0], data.size());
        
        soundElement->setAttribute("data", encodedSound.toString());
        soundElement->setAttribute("size", (int) data.size());
    }
    
    MemoryOutputStream encodedAps;
    Base64::convertToBase64(encodedAps, &apsBytes[0], apsBytes.size());
    
    auto mpc_aps = new XmlElement("MPC-APS");
    root->addChildElement(mpc_aps);
    
    mpc_aps->setAttribute("data", encodedAps.toString());
    mpc_aps->setAttribute("size", (int) apsBytes.size());
    
    AllParser allParser(mpc, "stateinfo");
    auto allBytes = allParser.getBytes();
    
    MemoryOutputStream encodedAll;
    Base64::convertToBase64(encodedAll, &allBytes[0], allBytes.size());
    
    auto mpc_all = new XmlElement("MPC-ALL");
    root->addChildElement(mpc_all);
    mpc_all->setAttribute("data", encodedAll.toString());
    mpc_all->setAttribute("size", (int) allBytes.size());
    
    copyXmlToBinary(*xml, destData);
}

void VmpcAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // MLOG("setStateInformation");
    unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    auto vmpcAutoSaveScreen = mpc.screens->get<VmpcAutoSaveScreen>("vmpc-auto-save");
    
    if (xmlState.get() != nullptr)
    {
        if (wrapperType == wrapperType_Standalone)
        {
            auto autoLoadOnStart = vmpcAutoSaveScreen->getAutoLoadOnStart();
            
            if (autoLoadOnStart == 0)
            {
                // Auto-load on start is Disabled
                return;
            }
            else if (autoLoadOnStart == 1)
            {
                // The user wants to be asked
                auto result = AlertWindow::showOkCancelBox (
                                                            AlertWindow::InfoIcon,
                                                            "Continue previous VMPC2000XL session?",
                                                            "An auto-saved previous session was found.",
                                                            "Forget and start new session",
                                                            "Continue session");
                if (result)
                {
                    // MLOG("Ignoring auto-saved session");
                    // We ignore the fact that a previously saved session exists,
                    // but we restore the user's window size.
                    auto juce_ui = xmlState->getChildByName("JUCE-UI");
                    if (juce_ui != nullptr)
                    {
                        lastUIWidth = juce_ui->getIntAttribute("w", 1298 / 2);
                        lastUIHeight = juce_ui->getIntAttribute("h", 994 / 2);
                    }
                    return;
                }
                else
                {
                    // MLOG("Continuing auto-saved session");
                    // We may continue the below routine.
                }
            }
            else if (autoLoadOnStart == 2)
            {
                // The user always wants to load auto-saved sessions.
                // We may continue the below routine.
            }
        }
        
        auto juce_ui = xmlState->getChildByName("JUCE-UI");
        
        if (juce_ui != nullptr)
        {
            lastUIWidth = juce_ui->getIntAttribute("w", 1298 / 2);
            lastUIHeight = juce_ui->getIntAttribute("h", 994 / 2);
        }
        
        auto mpc_ui = xmlState->getChildByName("MPC-UI");

        if (mpc_ui != nullptr)
        {
            auto currentDir = mpc_ui->getStringAttribute("currentDir").toStdString();
            auto storesPath = mpc::Paths::storesPath() + "MPC2000XL";
            auto resPathIndex = currentDir.find(storesPath);
            
            if (resPathIndex != string::npos)
            {
                auto trimmedCurrentDir = currentDir.substr(resPathIndex + storesPath.length());
                auto splitTrimmedDir = StrUtil::split(trimmedCurrentDir, FileUtil::getSeparator()[0]);
                
                for (auto& s : splitTrimmedDir)
                {
                    mpc.getDisk().lock()->moveForward(s);
                    mpc.getDisk().lock()->initFiles();
                }
            }
        }
        
        auto decodeBase64 = [](XmlElement* element) {
            MemoryOutputStream decoded;
            Base64::convertFromBase64(decoded, element->getStringAttribute("data"));
            auto decodedData = (char*) (decoded.getData());
            return vector<char>(decodedData, decodedData + element->getIntAttribute("size"));
        };
        
        auto mpc_aps = xmlState->getChildByName("MPC-APS");

        if (mpc_aps != nullptr)
        {
            vector<char> apsData = decodeBase64(mpc_aps);
            if (apsData.size() > 0)
            {
                ApsParser apsParser(mpc, apsData, "auto-state-from-xml");
                // We don't want the APS loader to attempt to load the sounds
                // from the file system. We load them manually from the
                // decode project state.
                auto withoutSounds = true;
                
                // We don't need popups to appear in the LCD UI.
                auto headless = true;
                
                ApsLoader::loadFromParsedAps(apsParser, mpc, withoutSounds, headless);
            }
            
            int counter = 0;
            
            auto candidateName = "sound" + to_string(counter);
            auto candidate = xmlState->getChildByName(candidateName);
            
            while (candidate != nullptr)
            {
                auto sndData = decodeBase64(candidate);
                SndReader sndReader(sndData);
                
                auto sound = mpc.getSampler().lock()->addSound(sndReader.getSampleRate()).lock();
                sound->setMono(sndReader.isMono());
                sndReader.readData(*sound->getSampleData());
                sound->setName(sndReader.getName());
                sound->setTune(sndReader.getTune());
                sound->setLevel(sndReader.getLevel());
                sound->setStart(sndReader.getStart());
                sound->setEnd(sndReader.getEnd());
                sound->setLoopTo(sound->getEnd() - sndReader.getLoopLength());
                sound->setBeatCount(sndReader.getNumberOfBeats());
                sound->setLoopEnabled(sndReader.isLoopEnabled());
                sound->setMemoryIndex(counter);
                
                counter++;
                
                candidateName = "sound" + to_string(counter);
                candidate = xmlState->getChildByName(candidateName);
            }
        }
        
        auto mpc_all = xmlState->getChildByName("MPC-ALL");

        if (mpc_all != nullptr)
        {
            vector<char> allData = decodeBase64(mpc_all);
            
            if (allData.size() > 0)
            {
                AllParser allParser(mpc, allData);
                AllLoader::loadEverythingFromAllParser(mpc, allParser);
            }
            
            auto screen = mpc_ui->getStringAttribute("screen").toStdString();
            mpc.getLayeredScreen().lock()->openScreen(screen);
            
            auto focus = mpc_ui->getStringAttribute("focus").toStdString();
            
            if (focus.length() > 0)
                mpc.getLayeredScreen().lock()->setFocus(focus);
            
            mpc.getLayeredScreen().lock()->setDirty();
            
            mpc.getSampler().lock()->setSoundIndex(mpc_ui->getIntAttribute("soundIndex"));
        }
    }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VmpcAudioProcessor();
}
