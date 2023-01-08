#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "version.h"

#include "gui/VmpcLookAndFeel.h"
#include "lcdgui/screens/VmpcSettingsScreen.hpp"
#include "AutoSave.hpp"

#include <audiomidi/AudioMidiServices.hpp>
#include <audiomidi/DiskRecorder.hpp>
#include <audiomidi/SoundRecorder.hpp>
#include <audiomidi/MpcMidiOutput.hpp>
#include <audiomidi/MpcMidiInput.hpp>
#include <audiomidi/MidiClockEmitter.hpp>

#include <file/aps/ApsParser.hpp>
#include <file/all/AllParser.hpp>
#include <file/sndwriter/SndWriter.hpp>
#include <file/sndreader/SndReader.hpp>
#include <disk/ApsLoader.hpp>
#include <disk/AllLoader.hpp>
#include <disk/AbstractDisk.hpp>

#include <Paths.hpp>
#include <sequencer/Sequencer.hpp>

#include <lcdgui/screens/SyncScreen.hpp>
#include <lcdgui/screens/window/DirectoryScreen.hpp>

// ctoot
#include <audio/server/NonRealTimeAudioServer.hpp>
#include <midi/core/ShortMessage.hpp>

// moduru
#include <lang/StrUtil.hpp>
#include <file/FileUtil.hpp>

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;
using namespace mpc::lcdgui::screens::window;
using namespace mpc::file::aps;
using namespace mpc::file::all;
using namespace mpc::file::sndwriter;
using namespace mpc::file::sndreader;
using namespace mpc::disk;

using namespace ctoot::midi::core;

using namespace moduru::lang;
using namespace moduru::file;

VmpcAudioProcessor::VmpcAudioProcessor()
: AudioProcessor (juce::PluginHostType::jucePlugInClientCurrentWrapperType == juce::AudioProcessor::wrapperType_AudioUnitv3 ?
                  BusesProperties()
                  .withOutput("STEREO OUT", juce::AudioChannelSet::stereo(), true)
                  :
                  BusesProperties()
                  .withInput("RECORD IN",  juce::AudioChannelSet::stereo(), true)
                  .withOutput("STEREO OUT", juce::AudioChannelSet::stereo(), true)
                  .withOutput("MIX OUT 1/2", juce::AudioChannelSet::stereo(), false)
                  .withOutput("MIX OUT 3/4", juce::AudioChannelSet::stereo(), false)
                  .withOutput("MIX OUT 5/6", juce::AudioChannelSet::stereo(), false)
                  .withOutput("MIX OUT 7/8", juce::AudioChannelSet::stereo(), false)
                  )
{
    lookAndFeel = new VmpcLookAndFeel();
    juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel);

    time_t currentTime = time(nullptr);
  struct tm* currentLocalTime = localtime(&currentTime);
  auto timeString = std::string(asctime(currentLocalTime));

  moduru::Logger::l.setPath(mpc::Paths::logFilePath());
  moduru::Logger::l.log("\n\n-= VMPC2000XL v" + std::string(version::get()) + " " + timeString.substr(0, timeString.length() - 1) + " =-\n");

  mpc.init(1, 5);

  if (juce::PluginHostType::jucePlugInClientCurrentWrapperType != juce::AudioProcessor::wrapperType_LV2)
  {
      mpc.getDisk()->initFiles();
  }

  if (juce::JUCEApplication::isStandaloneApp())
  {
    mpc::AutoSave::restoreAutoSavedState(mpc);
  }
}

VmpcAudioProcessor::~VmpcAudioProcessor()
{
    if (juce::JUCEApplication::isStandaloneApp())
    {
        mpc::AutoSave::storeAutoSavedState(mpc);
    }

    delete lookAndFeel;
}

const juce::String VmpcAudioProcessor::getName() const
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
  if (wrapperType == wrapperType_AudioUnitv3) return false;
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

void VmpcAudioProcessor::setCurrentProgram (int /* index */)
{
}

const juce::String VmpcAudioProcessor::getProgramName (int /* index */)
{
  return {};
}

void VmpcAudioProcessor::changeProgramName (int /* index */, const juce::String& /* newName */)
{
}

void VmpcAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
  auto seq = mpc.getSequencer();
  bool seqWasPlaying = seq->isPlaying();
  bool seqWasOverdubbing = seq->isOverDubbing();
  bool seqWasRecording = seq->isRecording();
  bool countWasEnabled = seq->isCountEnabled();

  if (seqWasPlaying)
  {
      seq->stop();
  }

  auto ams = mpc.getAudioMidiServices();
  ams->getMidiClockEmitter()->setSampleRate(static_cast<unsigned int>(sampleRate));
  auto server = ams->getAudioServer();
  server->setSampleRate(static_cast<int>(sampleRate));
  server->resizeBuffers(samplesPerBlock);

  seq->setCountEnabled(false);

  if (seqWasOverdubbing)
  {
      seq->overdub();
  }
  else if (seqWasRecording)
  {
      seq->rec();
  }
  else if (seqWasPlaying)
  {
      seq->play();
  }

  if (countWasEnabled)
  {
      seq->setCountEnabled(true);
  }

  monoToStereoBufferIn.clear();
  monoToStereoBufferIn.setSize(2, samplesPerBlock);
  monoToStereoBufferOut.clear();
  monoToStereoBufferOut.setSize(2, samplesPerBlock);
}

void VmpcAudioProcessor::releaseResources()
{
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

bool VmpcAudioProcessor::isBusesLayoutSupported (const BusesLayout&) const
{
  return true;
}

void VmpcAudioProcessor::processMidiIn(juce::MidiBuffer& midiMessages) {

  for (const auto& meta : midiMessages)
  {
    const auto& m = meta.getMessage();
    int timeStamp = static_cast<int>(m.getTimeStamp());
    int velocity = m.getVelocity();
    std::shared_ptr<ShortMessage> tootMsg;

    if (m.isNoteOn())
    {
      tootMsg = std::make_shared<ShortMessage>();
      tootMsg->setMessage(ShortMessage::NOTE_ON, m.getChannel() - 1, m.getNoteNumber(), velocity);
    }
    else if (m.isNoteOff())
    {
        tootMsg = std::make_shared<ShortMessage>();
        tootMsg->setMessage(ShortMessage::NOTE_OFF, m.getChannel() - 1, m.getNoteNumber(), 0);
    }
    else if (m.isController())
    {
        tootMsg = std::make_shared<ShortMessage>();
        tootMsg->setMessage(ShortMessage::CONTROL_CHANGE, m.getChannel() - 1, m.getControllerNumber(), m.getControllerValue());
    }
    else if (m.isAftertouch())
    {
        tootMsg = std::make_shared<ShortMessage>();
        tootMsg->setMessage(ShortMessage::POLY_PRESSURE, m.getChannel() - 1, m.getNoteNumber(), m.getAfterTouchValue());
    }
    else if (m.isChannelPressure())
    {
        tootMsg = std::make_shared<ShortMessage>();
        tootMsg->setMessage(ShortMessage::CHANNEL_PRESSURE, m.getChannel() - 1, m.getChannelPressureValue(), 0);
    }

    if (tootMsg)
    {
        mpc.getMpcMidiInput(0)->transport(tootMsg.get(), timeStamp);
    }
  }
}

void VmpcAudioProcessor::processMidiOut(juce::MidiBuffer& midiMessages)
{
    midiMessages.clear();

    static auto processMsg = [&midiMessages](std::shared_ptr<ShortMessage>& msg) {
        juce::MidiMessage juceMsg;
        bool compatibleMsg = false;

        if (msg->getCommand() == ShortMessage::NOTE_ON || msg->getCommand() == ShortMessage::NOTE_OFF)
        {
            juce::uint8 velo = (juce::uint8) msg->getData2();

            juceMsg = velo == 0 ? juce::MidiMessage::noteOff(msg->getChannel() + 1, msg->getData1())
                                                  : juce::MidiMessage::noteOn(msg->getChannel() + 1, msg->getData1(),
                                                                              juce::uint8(velo));
            compatibleMsg = true;
        }
        else if (msg->getStatus() == ShortMessage::TIMING_CLOCK)
        {
            juceMsg = juce::MidiMessage::midiClock();
            compatibleMsg = true;
        }
        else if (msg->getStatus() == ShortMessage::START)
        {
            juceMsg = juce::MidiMessage::midiStart();
            compatibleMsg = true;
        }
        else if (msg->getStatus() == ShortMessage::STOP)
        {
            juceMsg = juce::MidiMessage::midiStop();
            compatibleMsg = true;
        }
        else if (msg->getStatus() == ShortMessage::CONTINUE)
        {
            juceMsg = juce::MidiMessage::midiContinue();
            compatibleMsg = true;
        }

        if (compatibleMsg)
        {
            midiMessages.addEvent(juceMsg, msg->bufferPos);
        }
    };

    const auto outputAEventCount = mpc.getMidiOutput()->dequeueOutputA(midiOutputBuffer);

    for (unsigned int i = 0; i < outputAEventCount; i++)
    {
        processMsg(midiOutputBuffer[i]);
    }

    // In JUCE we only have 1 set of 16 MIDI channels as far as I know.
    // The MPC2000XL has 2 of those -- MIDI OUT A and MIDI OUT B. The below shows some
    // example processing, but it's commented out, to restrict MIDI out processing
    // to just MIDI OUT A.

//    const auto outputBEventCount = mpc.getMidiOutput()->dequeueOutputB(midiOutputBuffer);
//
//    for (int i = 0; i < outputBEventCount; i++)
//    {
//        processMsg(midiOutputBuffer[i]);
//    }
}

void VmpcAudioProcessor::processTransport()
{
  if (juce::JUCEApplication::isStandaloneApp())
  {
    return;
  }

  auto syncScreen = mpc.screens->get<SyncScreen>("sync");

  bool syncEnabled = syncScreen->getModeIn() == 1;

  if (syncEnabled)
  {
    auto info = getPlayHead()->getPosition();
    double tempo = info->getBpm().orFallback(120);

    if (tempo != m_Tempo || mpc.getSequencer()->getTempo() != tempo)
    {
      mpc.getSequencer()->setTempo(tempo);
      m_Tempo = tempo;
    }

    bool isPlaying = info->getIsPlaying();

    if (!wasPlaying && isPlaying)
    {
      mpc.getSequencer()->playFromStart();
    }

    if (wasPlaying && !isPlaying) {
      mpc.getSequencer()->stop();
    }
    wasPlaying = isPlaying;
  }
}

void VmpcAudioProcessor::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
  juce::ScopedNoDenormals noDenormals;

  const int totalNumInputChannels = getTotalNumInputChannels();
  const int totalNumOutputChannels = getTotalNumOutputChannels();
  auto audioMidiServices = mpc.getAudioMidiServices();
  auto server = audioMidiServices->getAudioServer();

  if (!server->isRunning())
  {
    for (int i = 0; i < totalNumInputChannels; ++i)
    {
      buffer.clear(i, 0, buffer.getNumSamples());
    }
    return;
  }

  audioMidiServices->changeBounceStateIfRequired();
  audioMidiServices->changeSoundRecorderStateIfRequired();
  audioMidiServices->switchMidiControlMappingIfRequired();

  if (!server->isRealTime())
  {
    for (int i = 0; i < totalNumInputChannels; ++i)
      buffer.clear(i, 0, buffer.getNumSamples());

    return;
  }

  processTransport();
  processMidiIn(midiMessages);

  auto chDataIn = buffer.getArrayOfReadPointers();
  auto chDataOut = buffer.getArrayOfWritePointers();
  int totalNumInputChannelsFinal = totalNumInputChannels;
  int totalNumOutputChannelsFinal = totalNumOutputChannels;

  if (totalNumInputChannels == 1)
  {
    monoToStereoBufferIn.clear();
    monoToStereoBufferIn.copyFrom(0, 0, buffer.getReadPointer(0), buffer.getNumSamples());
    monoToStereoBufferIn.copyFrom(1, 0, buffer.getReadPointer(0), buffer.getNumSamples());
    chDataIn = monoToStereoBufferIn.getArrayOfReadPointers();
    totalNumInputChannelsFinal = 2;
  }

  if (totalNumOutputChannels == 1)
  {
    monoToStereoBufferOut.clear();
    chDataOut = monoToStereoBufferOut.getArrayOfWritePointers();
    totalNumOutputChannelsFinal = 2;
  }

  server->work(chDataIn, chDataOut, buffer.getNumSamples(), totalNumInputChannelsFinal, totalNumOutputChannelsFinal);

  processMidiOut(midiMessages);

  if (totalNumOutputChannels < 1)
  {
    buffer.clear();
  }
  else if (totalNumOutputChannels == 1)
  {
    buffer.copyFrom(0, 0, monoToStereoBufferOut.getReadPointer(0), buffer.getNumSamples());
  }
}

bool VmpcAudioProcessor::hasEditor() const
{
  return true;
}

juce::AudioProcessorEditor* VmpcAudioProcessor::createEditor()
{
  mpc.getLayeredScreen()->setDirty();
  return new VmpcAudioProcessorEditor (*this);
}

void VmpcAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto editor = getActiveEditor();
    auto root = std::make_shared<juce::XmlElement>("root");

    auto juce_ui = new juce::XmlElement("JUCE-UI");
    root->addChildElement(juce_ui);

    if (editor != nullptr)
    {
        auto w = editor->getWidth();
        auto h = editor->getHeight();
        juce_ui->setAttribute("w", w);
        juce_ui->setAttribute("h", h);
    }

    if (juce::JUCEApplication::isStandaloneApp())
    {
        copyXmlToBinary(*root.get(), destData);
        return;
    }

    auto layeredScreen = mpc.getLayeredScreen();

    auto screen = layeredScreen->getCurrentScreenName();
    auto previousScreen = layeredScreen->getPreviousScreenName();
    auto previousSamplerScreen = mpc.getPreviousSamplerScreenName();
    auto focus = mpc.getLayeredScreen()->getFocus();
    auto soundIndex = mpc.getSampler()->getSoundIndex();
    auto lastPressedPad = mpc.getPad();
    auto lastPressedNote = mpc.getNote();

    auto mpc_ui = new juce::XmlElement("MPC-UI");
    root->addChildElement(mpc_ui);

    mpc_ui->setAttribute("screen", screen);
    mpc_ui->setAttribute("previousScreen", previousScreen);
    mpc_ui->setAttribute("previousSamplerScreen", previousSamplerScreen);
    mpc_ui->setAttribute("focus", focus);
    mpc_ui->setAttribute("soundIndex", soundIndex);
    mpc_ui->setAttribute("lastPressedNote", lastPressedNote);
    mpc_ui->setAttribute("lastPressedPad", lastPressedPad);
    mpc_ui->setAttribute("currentDir", mpc.getDisk()->getAbsolutePath());

    ApsParser apsParser(mpc, "stateinfo");
    auto apsBytes = apsParser.getBytes();
    auto sounds = mpc.getSampler()->getSounds();

    for (size_t i = 0; i < sounds.size(); i++)
    {
        auto elementName = "sound" + std::to_string(i);
        auto soundElement = new juce::XmlElement(elementName.c_str());
        root->addChildElement(soundElement);

        auto sound = sounds[i];
        SndWriter sndWriter(sound.get());
        auto data = sndWriter.getSndFileArray();

        juce::MemoryOutputStream encodedSound;
        juce::Base64::convertToBase64(encodedSound, &data[0], data.size());

        soundElement->setAttribute("data", encodedSound.toString());
        soundElement->setAttribute("size", (int) data.size());
    }

    juce::MemoryOutputStream encodedAps;
    juce::Base64::convertToBase64(encodedAps, &apsBytes[0], apsBytes.size());

    auto mpc_aps = new juce::XmlElement("MPC-APS");
    root->addChildElement(mpc_aps);

    mpc_aps->setAttribute("data", encodedAps.toString());
    mpc_aps->setAttribute("size", (int) apsBytes.size());

    AllParser allParser(mpc);
    auto allBytes = allParser.getBytes();

    juce::MemoryOutputStream encodedAll;
    juce::Base64::convertToBase64(encodedAll, &allBytes[0], allBytes.size());

    auto mpc_all = new juce::XmlElement("MPC-ALL");
    root->addChildElement(mpc_all);
    mpc_all->setAttribute("data", encodedAll.toString());
    mpc_all->setAttribute("size", (int) allBytes.size());

    copyXmlToBinary(*root.get(), destData);
}

void VmpcAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::shared_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    auto juce_ui = xmlState->getChildByName("JUCE-UI");

    if (juce_ui != nullptr)
    {
        lastUIWidth = juce_ui->getIntAttribute("w", 1298 / 2);
        lastUIHeight = juce_ui->getIntAttribute("h", 994 / 2);
    }

    if (juce::JUCEApplication::isStandaloneApp())
    {
        return;
    }

    auto decodeBase64 = [](juce::XmlElement *element) {
        juce::MemoryOutputStream decoded;
        juce::Base64::convertFromBase64(decoded, element->getStringAttribute("data"));
        auto decodedData = (char *) (decoded.getData());
        return std::vector<char>(decodedData, decodedData + element->getIntAttribute("size"));
    };

    auto mpc_aps = xmlState->getChildByName("MPC-APS");

    if (mpc_aps != nullptr)
    {
        std::vector<char> apsData = decodeBase64(mpc_aps);
        if (apsData.size() > 0)
        {
            ApsParser apsParser(mpc, apsData);
            // We don't want the APS loader to attempt to load the sounds
            // from the file system. We load them manually from the
            // decode project state.
            auto withoutSounds = true;

            // We don't need popups to appear in the LCD UI.
            auto headless = true;

            ApsLoader::loadFromParsedAps(apsParser, mpc, withoutSounds, headless);
        }

        int counter = 0;

        auto candidateName = "sound" + std::to_string(counter);
        auto candidate = xmlState->getChildByName(candidateName);

        while (candidate != nullptr)
        {
            auto sndData = decodeBase64(candidate);
            SndReader sndReader(sndData);

            auto sound = mpc.getSampler()->addSound(sndReader.getSampleRate());
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

            counter++;

            candidateName = "sound" + std::to_string(counter);
            candidate = xmlState->getChildByName(candidateName);
        }
    }

    auto mpc_all = xmlState->getChildByName("MPC-ALL");

    if (mpc_all != nullptr)
    {
        std::vector<char> allData = decodeBase64(mpc_all);

        if (allData.size() > 0)
        {
            AllParser allParser(mpc, allData);
            AllLoader::loadEverythingFromAllParser(mpc, allParser);
        }
    }

    auto mpc_ui = xmlState->getChildByName("MPC-UI");

    if (mpc_ui != nullptr)
    {
        auto currentDir = mpc_ui->getStringAttribute("currentDir").toStdString();
        auto storesPath = mpc::Paths::storesPath() + "MPC2000XL";
        auto resPathIndex = currentDir.find(storesPath);

        if (resPathIndex != std::string::npos)
        {
            auto trimmedCurrentDir = currentDir.substr(resPathIndex + storesPath.length());
            auto splitTrimmedDir = StrUtil::split(trimmedCurrentDir, FileUtil::getSeparator()[0]);

            for (auto &s: splitTrimmedDir)
            {
                mpc.getDisk()->moveForward(s);
                mpc.getDisk()->initFiles();
            }
        }

        mpc.getSampler()->setSoundIndex(mpc_ui->getIntAttribute("soundIndex"));
        mpc.setNote(mpc_ui->getIntAttribute("lastPressedNote"));
        mpc.setPad(static_cast<unsigned char>(mpc_ui->getIntAttribute("lastPressedPad")));

        auto previousSamplerScreen = mpc_ui->getStringAttribute("previousSamplerScreen").toStdString();
        mpc.setPreviousSamplerScreenName(previousSamplerScreen);

        auto screen = mpc_ui->getStringAttribute("screen").toStdString();
        auto previousScreen = mpc_ui->getStringAttribute("previousScreen").toStdString();
        auto layeredScreen = mpc.getLayeredScreen();

        auto currentScreen = layeredScreen->getCurrentScreenName();

        layeredScreen->openScreen(previousSamplerScreen.empty() ? "sequencer" : previousSamplerScreen);
        layeredScreen->Draw();
        layeredScreen->openScreen(previousScreen);
        layeredScreen->Draw();

        auto directoryScreen = mpc.screens->get<DirectoryScreen>("directory");
        directoryScreen->setPreviousScreenName(previousScreen == "save" ? "save" : "load");

        layeredScreen->openScreen(screen);
        auto focus = mpc_ui->getStringAttribute("focus").toStdString();
        layeredScreen->Draw();

        if (!focus.empty())
        {
            layeredScreen->setFocus(focus);
        }

        if (currentScreen == "vmpc-known-controller-detected")
        {
            layeredScreen->openScreen(currentScreen);
            layeredScreen->Draw();
        }

        layeredScreen->setDirty();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new VmpcAudioProcessor();
}
