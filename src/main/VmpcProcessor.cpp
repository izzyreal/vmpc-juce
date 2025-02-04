#include "VmpcProcessor.hpp"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "version.h"

#include "lcdgui/screens/VmpcSettingsScreen.hpp"
#include "AutoSave.hpp"

#include <audiomidi/AudioMidiServices.hpp>
#include <audiomidi/DiskRecorder.hpp>
#include <audiomidi/MidiInput.hpp>
#include <audiomidi/MidiOutput.hpp>
#include <audiomidi/SoundRecorder.hpp>

#include <file/aps/ApsParser.hpp>
#include <file/all/AllParser.hpp>
#include <file/sndwriter/SndWriter.hpp>
#include <file/sndreader/SndReader.hpp>
#include <disk/ApsLoader.hpp>
#include <disk/AllLoader.hpp>
#include <disk/AbstractDisk.hpp>

#include <sequencer/Sequencer.hpp>
#include <sequencer/ExternalClock.hpp>

#include <lcdgui/screens/SyncScreen.hpp>
#include <lcdgui/screens/window/DirectoryScreen.hpp>

#include <engine/audio/server/NonRealTimeAudioServer.hpp>
#include <engine/midi/ShortMessage.hpp>
#include <string>

#include "VmpcEditor.hpp"

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;
using namespace mpc::lcdgui::screens::window;
using namespace mpc::file::aps;
using namespace mpc::file::all;
using namespace mpc::file::sndwriter;
using namespace mpc::file::sndreader;
using namespace mpc::disk;

using namespace mpc::engine::midi;

using namespace vmpc_juce;


VmpcProcessor::VmpcProcessor()
: AudioProcessor (getBusesProperties())
{
    time_t currentTime = time(nullptr);
  struct tm* currentLocalTime = localtime(&currentTime);
  auto timeString = std::string(asctime(currentLocalTime));

  mpc::Logger::l.setPath(mpc.paths->logFilePath().string());
  mpc::Logger::l.log("\n\n-= VMPC2000XL v" + std::string(version::get()) + " " + timeString.substr(0, timeString.length() - 1) + " =-\n");

  mpc.init();

  if (juce::PluginHostType::jucePlugInClientCurrentWrapperType != juce::AudioProcessor::wrapperType_LV2)
  {
      mpc.getDisk()->initFiles();
  }

  if (juce::JUCEApplication::isStandaloneApp())
  {
    mpc::AutoSave::restoreAutoSavedState(mpc);
  }
  else
  {
      auto syncScreen = mpc.screens->get<SyncScreen>("sync");
      syncScreen->modeIn = 1;
      mpc.setPluginModeEnabled(true);
  }
  
  mpc.startMidiDeviceDetector();
}

VmpcProcessor::~VmpcProcessor()
{
    if (juce::JUCEApplication::isStandaloneApp())
    {
        mpc::AutoSave::storeAutoSavedState(mpc);
    }
}

const juce::String VmpcProcessor::getName() const
{
  return JucePlugin_Name;
}

bool VmpcProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool VmpcProcessor::producesMidi() const
{
  if (wrapperType == wrapperType_AudioUnitv3) return false;
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool VmpcProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double VmpcProcessor::getTailLengthSeconds() const
{
  return 0.0;
}

int VmpcProcessor::getNumPrograms()
{
  return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
  // so this should be at least 1, even if you're not really implementing programs.
}

int VmpcProcessor::getCurrentProgram()
{
  return 0;
}

void VmpcProcessor::setCurrentProgram (int /* index */)
{
}

const juce::String VmpcProcessor::getProgramName (int /* index */)
{
  return {};
}

void VmpcProcessor::changeProgramName (int /* index */, const juce::String& /* newName */)
{
}

void VmpcProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
  mpc.panic();
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
  auto server = ams->getAudioServer();
  server->setSampleRate(static_cast<int>(sampleRate));
  server->resizeBuffers(samplesPerBlock);
  ams->getFrameSequencer()->setSampleRate(static_cast<unsigned int>(sampleRate));

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
}

bool VmpcProcessor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    int monoInputCount = 0, monoOutputCount = 0;

    for (int i = 0; i < layout.getBuses(true).size(); i++)
    {
        if (layout.getNumChannels(true, i) > 2)
        {
            return false;
        }

        monoInputCount += layout.getNumChannels(true, i);
    }

    for (int i = 0; i < layout.getBuses(false).size(); i++)
    {
        if (layout.getNumChannels(false, i) > 2)
        {
            return false;
        }

        monoOutputCount += layout.getNumChannels(false, i);
    }
    
    return monoOutputCount >= 2 && monoInputCount <= 4 && monoOutputCount <= 10;
}

void VmpcProcessor::processMidiIn(juce::MidiBuffer& midiMessages) {

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
    else if (m.isMidiClock())
    {
        tootMsg = std::make_shared<ShortMessage>();
        tootMsg->setMessage(ShortMessage::TIMING_CLOCK);
    }
    else if (m.isMidiStart())
    {
        tootMsg = std::make_shared<ShortMessage>();
        tootMsg->setMessage(ShortMessage::START);
    }
    else if (m.isMidiContinue())
    {
        tootMsg = std::make_shared<ShortMessage>();
        tootMsg->setMessage(ShortMessage::CONTINUE);
    }
    else if (m.isMidiStop())
    {
        tootMsg = std::make_shared<ShortMessage>();
        tootMsg->setMessage(ShortMessage::STOP);
    }

    if (tootMsg)
    {
        mpc.getMpcMidiInput(0)->transport(tootMsg.get(), timeStamp);
    }
  }
}

void processMidiOutMsg(juce::MidiBuffer& midiMessages, std::shared_ptr<ShortMessage>& msg)
{
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
}

void VmpcProcessor::processMidiOut(juce::MidiBuffer& midiMessages, bool discard)
{
    midiMessages.clear();

    const auto outputAEventCount = mpc.getMidiOutput()->dequeueOutputA(midiOutputBuffer);

    if (discard)
    {
        return;
    }

    for (unsigned int i = 0; i < outputAEventCount; i++)
    {
        processMidiOutMsg(midiMessages, midiOutputBuffer[i]);
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

void VmpcProcessor::processTransport()
{
  if (juce::JUCEApplication::isStandaloneApp())
  {
    return;
  }

  auto syncScreen = mpc.screens->get<SyncScreen>("sync");

  bool syncEnabled = syncScreen->getModeIn() == 1;

  if (syncEnabled)
  {
    const auto info = getPlayHead()->getPosition();
    const double tempo = info->getBpm().orFallback(120);
    const bool isPlaying = info->getIsPlaying();

    if (tempo != m_Tempo || mpc.getSequencer()->getTempo() != tempo)
    {
      if (isPlaying)
      {
          mpc.getSequencer()->setTempo(tempo);
      }

      m_Tempo = tempo;
    }

    if (!wasPlaying && isPlaying)
    {
        mpc.getSequencer()->setSongModeEnabled(mpc.getLayeredScreen()->getCurrentScreenName() == "song");
        mpc.getSequencer()->playFromStart();
    }

    if (wasPlaying && !isPlaying)
    {
      mpc.getSequencer()->stop();
    }

    wasPlaying = isPlaying;
  }
}

void VmpcProcessor::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
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

  auto playHead = getPlayHead();

  if (playHead != nullptr)
  {
      auto info = playHead->getPosition();
      if (info->getIsPlaying())
      {
          auto ppqPos = info->getPpqPosition();
          if (ppqPos.hasValue())
          {
             mpc.getExternalClock()->clearTicks();
             mpc.getExternalClock()->computeTicksForCurrentBuffer(*ppqPos,
                                                                  *info->getPpqPositionOfLastBarStart(),
                                                                  buffer.getNumSamples(), getSampleRate(),
                                                                  m_Tempo);
         }
     }
  }

  auto chDataIn = buffer.getArrayOfReadPointers();
  auto chDataOut = buffer.getArrayOfWritePointers();

  std::vector<uint8_t> mpcMonoInputChannelIndices, mpcMonoOutputChannelIndices, hostInputChannelIndices, hostOutputChannelIndices;

  int hostIndex = 0;

  for (int i = 0; i < getBusCount(true); i++)
  {
      const auto bus = getBus(true, i);
      
      if (!bus->isEnabled())
      {
          continue;
      }

      if (bus->getBusIndex() < 1)
      {
          const bool busWasRequestedToBeStereoButIsMono = bus->getNumberOfChannels() == 1;
          
          hostInputChannelIndices.push_back(hostIndex++);
          mpcMonoInputChannelIndices.push_back(static_cast<uint8_t>(std::floor(bus->getBusIndex() / 2.f)));

          if (!busWasRequestedToBeStereoButIsMono)
          {
              hostInputChannelIndices.push_back(hostIndex++);
              mpcMonoInputChannelIndices.push_back(mpcMonoInputChannelIndices.back());
          }
      }
      else
      {
          jassert(bus->getNumberOfChannels() == 1);
          hostInputChannelIndices.push_back(hostIndex++);
          mpcMonoInputChannelIndices.push_back(static_cast<uint8_t>(std::floor((bus->getBusIndex() - 1) / 2.f)));
      }
  }

  if (hostInputChannelIndices.size() == 1)
  {
      hostInputChannelIndices.push_back(hostInputChannelIndices.front());
      mpcMonoInputChannelIndices.push_back(mpcMonoInputChannelIndices.front());
  }

  hostIndex = 0;

  int mpcMonoOutputCounter = 0;

  std::vector<int> channelIndicesThatWereWritten;

  for (int i = 0; i < getBusCount(false); i++)
  {
      if (mpcMonoOutputCounter >= 10)
      {
          mpcMonoOutputCounter -= 10;
      }

      const auto bus = getBus(false, i);

      if (!bus->isEnabled())
      {
          mpcMonoOutputCounter += bus->getLastEnabledLayout().size();
          continue;
      }

      if (bus->getBusIndex() < 5)
      {
          jassert(bus->getNumberOfChannels() == 2);
          channelIndicesThatWereWritten.push_back(bus->getChannelIndexInProcessBlockBuffer(0));
          hostOutputChannelIndices.push_back(bus->getChannelIndexInProcessBlockBuffer(0));
          channelIndicesThatWereWritten.push_back(bus->getChannelIndexInProcessBlockBuffer(1));
          hostOutputChannelIndices.push_back(bus->getChannelIndexInProcessBlockBuffer(1));
          mpcMonoOutputChannelIndices.push_back(mpcMonoOutputCounter);
          mpcMonoOutputChannelIndices.push_back(mpcMonoOutputCounter + 1);
      }
      else
      {
          jassert(bus->getNumberOfChannels() == 1);
          channelIndicesThatWereWritten.push_back(bus->getChannelIndexInProcessBlockBuffer(0));
          hostOutputChannelIndices.push_back(bus->getChannelIndexInProcessBlockBuffer(0));
          mpcMonoOutputChannelIndices.push_back(mpcMonoOutputCounter);
      }

      mpcMonoOutputCounter += bus->getNumberOfChannels();
  }

  server->work(chDataIn, chDataOut, buffer.getNumSamples(), mpcMonoInputChannelIndices, mpcMonoOutputChannelIndices, hostInputChannelIndices, hostOutputChannelIndices);

  // I've observed a crash in Ableton Live VST3 indicating what could be allocating MIDI events too soon.
  // So we give it a little bit of leeway of 10000 frames.
  if (framesProcessed < 10000)
  {
      framesProcessed += buffer.getNumSamples();
      processMidiOut(midiMessages, true);
  }
  else
  {
      processMidiOut(midiMessages, false);
  }

  if (totalNumOutputChannels < 1)
  {
    buffer.clear();
  }
  else
  {
      for (int i = 0; i < buffer.getNumChannels(); i++)
      {
          if (std::find(channelIndicesThatWereWritten.begin(), channelIndicesThatWereWritten.end(), i) != channelIndicesThatWereWritten.end())
          {
              continue;
          }

          buffer.clear(i, 0, buffer.getNumSamples());
      }
  }
}

bool VmpcProcessor::hasEditor() const
{
  return true;
}

juce::AudioProcessorEditor* VmpcProcessor::createEditor()
{
  mpc.getLayeredScreen()->setDirty();
  return new VmpcEditor (*this);
}

void VmpcProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto editor = getActiveEditor();
    auto root = std::make_shared<juce::XmlElement>("root");

    auto juce_ui = new juce::XmlElement("JUCE-UI");
    root->addChildElement(juce_ui);

    if (editor != nullptr)
    {
        auto w = editor->getWidth();
        auto h = editor->getHeight();
        juce_ui->setAttribute("vector_ui_width", w);
        juce_ui->setAttribute("vector_ui_height", h);
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
        auto& data = sndWriter.getSndFileArray();

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

void VmpcProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::shared_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    auto juce_ui = xmlState->getChildByName("JUCE-UI");

    if (juce_ui != nullptr)
    {
        lastUIWidth = juce_ui->getIntAttribute("vector_ui_width", lastUIWidth);
        lastUIHeight = juce_ui->getIntAttribute("vector_ui_height", lastUIHeight);
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
            ApsParser apsParser(apsData);
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

            auto sound = mpc.getSampler()->addSound(sndReader.getSampleRate(), "");

            if (sound == nullptr)
            {
                return;
            }

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
        auto currentDir = fs::path(mpc_ui->getStringAttribute("currentDir").toStdString());
        auto relativePath = fs::relative(currentDir, mpc.paths->defaultLocalVolumePath());

        for (auto& pathSegment : relativePath)
        {
            mpc.getDisk()->moveForward(pathSegment.string());
            mpc.getDisk()->initFiles();
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

juce::AudioProcessor::BusesProperties VmpcProcessor::getBusesProperties()
{

    auto result = juce::AudioProcessor::BusesProperties()
        .withInput("RECORD IN L/R",  juce::AudioChannelSet::stereo(), true)
        .withOutput("STEREO OUT L/R", juce::AudioChannelSet::stereo(), true);
    
    if (juce::PluginHostType::jucePlugInClientCurrentWrapperType == juce::AudioProcessor::WrapperType::wrapperType_AudioUnitv3)
    {
        return result;
    }

    const bool isStandalone = juce::JUCEApplication::isStandaloneApp();
    
    result = result
        .withOutput("MIX OUT 1/2", juce::AudioChannelSet::stereo(), isStandalone)
        .withOutput("MIX OUT 3/4", juce::AudioChannelSet::stereo(), isStandalone)
        .withOutput("MIX OUT 5/6", juce::AudioChannelSet::stereo(), isStandalone)
        .withOutput("MIX OUT 7/8", juce::AudioChannelSet::stereo(), isStandalone);

    if (isStandalone || juce::PluginHostType().isAbletonLive())
    {
        return result;
    }

    result = result
        .withInput("RECORD IN L",  juce::AudioChannelSet::mono(), false)
        .withInput("RECORD IN R",  juce::AudioChannelSet::mono(), false)
        .withOutput("STEREO OUT L", juce::AudioChannelSet::mono(), false)
        .withOutput("STEREO OUT R", juce::AudioChannelSet::mono(), false)
        .withOutput("MIX OUT 1", juce::AudioChannelSet::mono(), false)
        .withOutput("MIX OUT 2", juce::AudioChannelSet::mono(), false)
        .withOutput("MIX OUT 3", juce::AudioChannelSet::mono(), false)
        .withOutput("MIX OUT 4", juce::AudioChannelSet::mono(), false)
        .withOutput("MIX OUT 5", juce::AudioChannelSet::mono(), false)
        .withOutput("MIX OUT 6", juce::AudioChannelSet::mono(), false)
        .withOutput("MIX OUT 7", juce::AudioChannelSet::mono(), false)
        .withOutput("MIX OUT 8", juce::AudioChannelSet::mono(), false);
    
    return result;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new VmpcProcessor();
}
