#include "VmpcProcessor.hpp"

#include "VmpcEditor.hpp"
#include "JuceToMpcMidiEventConvertor.hpp"
#include "controller/ClientEventController.hpp"

#include "FloatUtil.hpp"

#include "ZipSaveTarget.hpp"
#include "DirectorySaveTarget.hpp"

#include <version.h>
#include <AutoSave.hpp>

#include <audiomidi/AudioMidiServices.hpp>
#include <audiomidi/DiskRecorder.hpp>
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
#include <sequencer/Clock.hpp>
#include <lcdgui/screens/SyncScreen.hpp>
#include <lcdgui/screens/window/DirectoryScreen.hpp>
#include <lcdgui/screens/VmpcSettingsScreen.hpp>
#include <engine/audio/server/NonRealTimeAudioServer.hpp>
#include <input/HostInputEvent.hpp>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/system/juce_PlatformDefs.h>
#include <juce_gui_basics/juce_gui_basics.h>

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

VmpcProcessor::VmpcProcessor() : AudioProcessor(getBusesProperties())
{
    mpcMonoOutputChannelIndicesToRender.reserve(18);
    hostOutputChannelIndicesToRender.reserve(18);
    previousHostOutputChannelIndicesToRender.reserve(18);
    possiblyActiveMpcMonoOutChannels.reserve(10);

    time_t currentTime = time(nullptr);
    struct tm *currentLocalTime = localtime(&currentTime);
    auto timeString = std::string(asctime(currentLocalTime));

    mpc::Logger::l.setPath(mpc.paths->logFilePath().string());
    mpc::Logger::l.log("\n\n-= VMPC2000XL v" + std::string(version::get()) +
                       " " + timeString.substr(0, timeString.length() - 1) +
                       " =-\n");

    mpc.init();

    if (juce::PluginHostType::jucePlugInClientCurrentWrapperType !=
        juce::AudioProcessor::wrapperType_LV2)
    {
        mpc.getDisk()->initFiles();
    }

    if (juce::JUCEApplication::isStandaloneApp())
    {
        const auto autosaveDir = mpc.paths->autoSavePath();
        auto saveTarget =
            std::make_shared<mpc::DirectorySaveTarget>(autosaveDir);
        mpc::AutoSave::restoreAutoSavedStateWithTarget(mpc, saveTarget);
    }
    else
    {
        auto syncScreen = mpc.screens->get<SyncScreen>();
        syncScreen->modeIn = 1;
        mpc.setPluginModeEnabled(true);
    }

    mpc.startMidiDeviceDetector();
}

VmpcProcessor::~VmpcProcessor()
{
    if (juce::JUCEApplication::isStandaloneApp())
    {
        const auto autosaveDir = mpc.paths->autoSavePath();
        auto saveTarget =
            std::make_shared<mpc::DirectorySaveTarget>(autosaveDir);
        mpc::AutoSave::storeAutoSavedStateWithTarget(mpc, saveTarget);
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
    if (wrapperType == wrapperType_AudioUnitv3)
    {
        return false;
    }
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
    return 1; // NB: some hosts don't cope very well if you tell them there are
              // 0 programs,
    // so this should be at least 1, even if you're not really implementing
    // programs.
}

int VmpcProcessor::getCurrentProgram()
{
    return 0;
}

void VmpcProcessor::setCurrentProgram(int /* index */) {}

const juce::String VmpcProcessor::getProgramName(int /* index */)
{
    return {};
}

void VmpcProcessor::changeProgramName(int /* index */,
                                      const juce::String & /* newName */)
{
}

void VmpcProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mpc.panic();
    auto seq = mpc.getSequencer();
    bool seqWasPlaying = seq->isPlaying();
    bool seqWasOverdubbing = seq->isOverdubbing();
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
    ams->getFrameSequencer()->setSampleRate(
        static_cast<unsigned int>(sampleRate));

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

    computeHostToMpcChannelMappings();

    previousHostOutputChannelIndicesToRender.clear();
    previousHostOutputChannelIndicesToRender.push_back(
        std::numeric_limits<int8_t>::max());

    // logActualBusLayout();
}

juce::AudioProcessor::BusesProperties VmpcProcessor::getBusesProperties()
{
    // Hosttypes don't seem to work at this point, but we may need to verify
    // that. First, let's make all decisions based on wrapper type, and then we
    // can further refine if necessary.
    typedef juce::AudioProcessor::WrapperType W;
    typedef juce::AudioChannelSet C;

    const auto wrapper =
        juce::PluginHostType::jucePlugInClientCurrentWrapperType;
    const bool isStandalone = juce::JUCEApplication::isStandaloneApp();

    if (isStandalone)
    {
        juce::AudioProcessor::BusesProperties result;
        result.addBus(false, "OUTPUT", C::discreteChannels(10));
        result.addBus(true, "RECORD", C::discreteChannels(2));
        return result;
    }

    const bool isAUv2 = wrapper == W::wrapperType_AudioUnit;
    const bool isAUv3 = wrapper == W::wrapperType_AudioUnitv3;

    int monoInCount;
    int stereoInCount;
    int monoOutCount;
    int stereoOutCount;

    if (isAUv2 || isAUv3)
    {
        monoInCount = 0;
        stereoInCount = 1;
        monoOutCount = 8;
        stereoOutCount = 5;
    }
    else /* if LV2 or VST3 */
    {
        monoInCount = 2;
        stereoInCount = 1;
        monoOutCount = 8;
        stereoOutCount = 5;
    }

    juce::AudioProcessor::BusesProperties result;

    for (int i = 0; i < stereoInCount; i++)
    {
        result = result.withInput("RECORD IN L/R", C::stereo(), true);
    }

    for (int i = 0; i < stereoOutCount; i++)
    {
        const auto name = i == 0 ? "STEREO OUT L/R"
                                 : ("MIX OUT " + std::to_string((i * 2) - 1) +
                                    "/" + std::to_string(i * 2));
        const bool enabledByDefault =
            i == 0 || isStandalone || isAUv2 || isAUv3;
        result = result.withOutput(name, C::stereo(), enabledByDefault);
    }

    for (int i = 0; i < monoInCount; i++)
    {
        result = result.withInput("RECORD IN " +
                                      std::string((i % 2 == 0) ? "L" : "R"),
                                  C::mono(), true);
    }

    for (int i = 0; i < monoOutCount; i++)
    {
        const auto name = "MIX OUT " + std::to_string(i + 1);
        const bool enabledByDefault = isStandalone || isAUv2 || isAUv3;
        result = result.withOutput(name, C::mono(), enabledByDefault);
    }

    return result;
}

bool VmpcProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    if (juce::JUCEApplication::isStandaloneApp())
    {
        return true;
    }

    if (!juce::JUCEApplication::isStandaloneApp())
    {
        if (layouts.getMainInputChannelSet() !=
                juce::AudioChannelSet::stereo() &&
            layouts.getMainInputChannelSet() !=
                juce::AudioChannelSet::disabled())
        {
            return false;
        }

        for (int i = 1; i < layouts.getBuses(true).size(); i++)
        {
            if (layouts.getChannelSet(true, i) !=
                    juce::AudioChannelSet::mono() &&
                layouts.getChannelSet(true, i) !=
                    juce::AudioChannelSet::disabled())
            {
                return false;
            }
        }
    }

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    {
        return false;
    }

    for (int i = 1; i < layouts.getBuses(false).size(); i++)
    {
        if (i >= 5)
        {
            break;
        }

        if (layouts.getChannelSet(false, i) !=
                juce::AudioChannelSet::stereo() &&
            layouts.getChannelSet(false, i) !=
                juce::AudioChannelSet::disabled())
        {
            return false;
        }
    }

    for (int i = 5; i < layouts.getBuses(false).size(); i++)
    {
        if (layouts.getChannelSet(false, i) != juce::AudioChannelSet::mono() &&
            layouts.getChannelSet(false, i) !=
                juce::AudioChannelSet::disabled())
        {
            return false;
        }
    }

    const std::function<int(const bool isInput)> getTotalChannelCountForBus =
        [&](const bool isInput)
    {
        int result = 0;
        for (int i = 0; i < layouts.getBuses(isInput).size(); i++)
        {
            result += layouts.getNumChannels(isInput, i);
        }
        return result;
    };

    const std::function<std::vector<int>(const bool isInput)> getChannelCounts =
        [&](const bool isInput)
    {
        std::vector<int> result;
        for (int i = 0; i < layouts.getBuses(isInput).size(); i++)
        {
            result.push_back(layouts.getNumChannels(isInput, i));
        }
        return result;
    };

    const std::function<int(const bool isInput, const int numChannels)>
        getBusCountForNumChannels =
            [&](const bool isInput, const int numChannels)
    {
        int result = 0;
        for (int i = 0; i < layouts.getBuses(isInput).size(); i++)
        {
            if (layouts.getNumChannels(isInput, i) == numChannels)
            {
                result++;
            }
        }
        return result;
    };

    typedef juce::AudioProcessor::WrapperType W;

    const auto wrapper =
        juce::PluginHostType::jucePlugInClientCurrentWrapperType;
    const bool isStandalone = juce::JUCEApplication::isStandaloneApp();
    const bool isAUv2 = wrapper == W::wrapperType_AudioUnit;
    const bool isAUv3 = wrapper == W::wrapperType_AudioUnitv3;
    const bool isVST3 = wrapper == W::wrapperType_VST3;
    const bool isLV2 = wrapper == W::wrapperType_LV2;

    const int monoInputCount = getBusCountForNumChannels(true, 1);
    const int monoOutputCount = getBusCountForNumChannels(false, 1);
    const int stereoInputCount = getBusCountForNumChannels(true, 2);
    const int stereoOutputCount = getBusCountForNumChannels(false, 2);
    const int totalNumInputChannels = getTotalChannelCountForBus(true);
    const int totalNumOutputChannels = getTotalChannelCountForBus(false);

    bool result = false;

    if (isAUv2 || isAUv3)
    {
        result = stereoInputCount == 1 && monoInputCount == 0 &&
                 stereoOutputCount == 1 && monoOutputCount == 0;
        result = result || (stereoOutputCount == 5 && monoOutputCount == 8 &&
                            stereoInputCount == 1 && monoInputCount == 0);
    }
    else if (isStandalone)
    {
        result = totalNumInputChannels <= 2 && totalNumOutputChannels <= 10;
    }
    else if (isVST3 || isLV2)
    {
        result = totalNumInputChannels <= 4 && totalNumOutputChannels <= 18;
    }

    return result;
}

void VmpcProcessor::processMidiIn(juce::MidiBuffer &midiMessages)
{
    for (const auto &meta : midiMessages)
    {
        const auto &m = meta.getMessage();

        mpc::client::event::ClientMidiEvent mpcMidiEvent =
            vmpc_juce::JuceToMpcMidiEventConvertor::convert(m);
        mpc.dispatchHostInput(mpc::input::HostInputEvent(mpcMidiEvent));
    }
}

static void processMidiOutMsg(juce::MidiBuffer &midiMessages,
                              std::shared_ptr<ShortMessage> &msg)
{
    juce::MidiMessage juceMsg;
    bool compatibleMsg = false;

    if (msg->getCommand() == ShortMessage::NOTE_ON ||
        msg->getCommand() == ShortMessage::NOTE_OFF)
    {
        juce::uint8 velo = (juce::uint8)msg->getData2();

        juceMsg = velo == 0 ? juce::MidiMessage::noteOff(msg->getChannel() + 1,
                                                         msg->getData1())
                            : juce::MidiMessage::noteOn(msg->getChannel() + 1,
                                                        msg->getData1(),
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

void VmpcProcessor::processMidiOut(juce::MidiBuffer &midiMessages, bool discard)
{
    midiMessages.clear();

    const auto outputAEventCount =
        mpc.getMidiOutput()->dequeueOutputA(midiOutputBuffer);

    if (discard)
    {
        return;
    }

    for (unsigned int i = 0; i < outputAEventCount; i++)
    {
        processMidiOutMsg(midiMessages, midiOutputBuffer[i]);
    }

    // In JUCE we only have 1 set of 16 MIDI channels as far as I know.
    // The MPC2000XL has 2 of those -- MIDI OUT A and MIDI OUT B. The below
    // shows some example processing, but it's commented out, to restrict MIDI
    // out processing to just MIDI OUT A.

    //    const auto outputBEventCount =
    //    mpc.getMidiOutput()->dequeueOutputB(midiOutputBuffer);
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

    auto syncScreen = mpc.screens->get<SyncScreen>();

    bool syncEnabled = syncScreen->getModeIn() == 1;

    if (const auto positionInfo = getPlayHead()->getPosition();
        positionInfo.hasValue() && syncEnabled)
    {
        const double tempo = positionInfo->getBpm().orFallback(120);
        const bool isPlaying = positionInfo->getIsPlaying();

        if (!nearlyEqual(tempo, m_Tempo) ||
            !nearlyEqual(mpc.getSequencer()->getTempo(), tempo))
        {
            mpc.getSequencer()->setTempo(tempo);
            m_Tempo = tempo;
        }

        const auto positionQuarterNotes =
            positionInfo->getPpqPosition().orFallback(0);

        if (!wasPlaying && isPlaying)
        {
            const bool shouldEnableSongMode =
                mpc.getLayeredScreen()->getCurrentScreenName() == "song";

            mpc.getSequencer()->setSongModeEnabled(shouldEnableSongMode);

            const bool inSongScreen =
                mpc.getLayeredScreen()->getCurrentScreenName() == "song";

            if (inSongScreen)
            {
                mpc.getSequencer()->moveWithinSong(positionQuarterNotes);
            }
            else
            {
                mpc.getSequencer()->move(positionQuarterNotes);
            }

            mpc.getSequencer()->play();
        }
        else if (wasPlaying && !isPlaying && mpc.getSequencer()->isPlaying())
        {
            mpc.getSequencer()->stop();
            previousPositionQuarterNotes =
                std::numeric_limits<double>::lowest();
        }
        else if (!isPlaying && !mpc.getSequencer()->isPlaying())
        {
            if (!nearlyEqual(positionQuarterNotes,
                             previousPositionQuarterNotes))
            {
                const bool inSongScreen =
                    mpc.getLayeredScreen()->getCurrentScreenName() == "song";

                if (inSongScreen)
                {
                    mpc.getSequencer()->moveWithinSong(positionQuarterNotes);
                }
                else
                {
                    mpc.getSequencer()->move(positionQuarterNotes);
                }

                previousPositionQuarterNotes = positionQuarterNotes;
            }
        }

        wasPlaying = isPlaying;
    }
}

void VmpcProcessor::computeMpcAndHostOutputChannelIndicesToRender()
{
    computePossiblyActiveMpcMonoOutChannels();

    mpcMonoOutputChannelIndicesToRender.clear();
    hostOutputChannelIndicesToRender.clear();

    for (size_t i = 0; i < mpcMonoOutputChannelIndices.size(); i++)
    {
        if (possiblyActiveMpcMonoOutChannels.count(
                mpcMonoOutputChannelIndices[i]) > 0)
        {
            mpcMonoOutputChannelIndicesToRender.push_back(
                mpcMonoOutputChannelIndices[i]);
            hostOutputChannelIndicesToRender.push_back(
                hostOutputChannelIndices[i]);
        }
    }

    for (auto &i : mpcMonoOutputChannelIndicesToRender)
    {
        assert(i >= 0);
    }
    for (auto &i : hostOutputChannelIndicesToRender)
    {
        assert(i >= 0);
    }
    for (auto &i : previousHostOutputChannelIndicesToRender)
    {
        assert(i >= 0);
    }
    for (auto &i : possiblyActiveMpcMonoOutChannels)
    {
        assert(i >= 0);
    }
}

static void propagateTransportInfo(mpc::sequencer::Clock &clock,
                                   const juce::AudioPlayHead *playHead,
                                   const uint32_t sampleRate,
                                   const uint16_t numSamples)
{
    const auto positionInfo = playHead->getPosition();
    const auto hostPositionQuarterNotes = positionInfo->getPpqPosition();
    const auto tempo = positionInfo->getBpm();
    const auto timeInSamples = positionInfo->getTimeInSamples();

    if (hostPositionQuarterNotes.hasValue() && tempo.hasValue() &&
        timeInSamples.hasValue())
    {
        clock.processBufferExternal(*hostPositionQuarterNotes, numSamples,
                                    static_cast<int>(sampleRate), *tempo,
                                    *timeInSamples);
    }
}

void VmpcProcessor::processBlock(juce::AudioSampleBuffer &buffer,
                                 juce::MidiBuffer &midiMessages)
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
        {
            buffer.clear(i, 0, buffer.getNumSamples());
        }

        return;
    }

    processTransport();
    processMidiIn(midiMessages);

    auto mpcClock = mpc.getClock();

    if (juce::JUCEApplication::isStandaloneApp())
    {
        if (mpc.getSequencer()->isPlaying())
        {
            mpcClock->generateTransportInfo(
                static_cast<float>(mpc.getSequencer()->getTempo()),
                static_cast<uint32_t>(getSampleRate()),
                static_cast<uint16_t>(buffer.getNumSamples()),
                mpc.getSequencer()->getPlayStartPositionQuarterNotes());
        }
    }
    else
    {
        const auto playHead = getPlayHead();

        const bool isPlaying = playHead != nullptr &&
                               playHead->getPosition().hasValue() &&
                               playHead->getPosition()->getIsPlaying();

        if (!isPlaying && mpc.getSequencer()->isPlaying())
        {
            mpcClock->generateTransportInfo(
                static_cast<float>(mpc.getSequencer()->getTempo()),
                static_cast<uint32_t>(getSampleRate()),
                static_cast<uint16_t>(buffer.getNumSamples()),
                mpc.getSequencer()->getPlayStartPositionQuarterNotes());
        }
        else if (isPlaying && mpc.getSequencer()->isPlaying())
        {
            propagateTransportInfo(
                *mpcClock, playHead, static_cast<uint32_t>(getSampleRate()),
                static_cast<uint16_t>(buffer.getNumSamples()));
        }
        else if (isPlaying)
        {
            const bool inSongScreen =
                mpc.getLayeredScreen()->getCurrentScreenName() == "song";

            if (inSongScreen)
            {
                mpc.getSequencer()->setPositionWithinSong(
                    mpcClock->getLastProcessedHostPositionQuarterNotes());
            }
            else
            {
                mpc.getSequencer()->setPosition(
                    mpcClock->getLastProcessedHostPositionQuarterNotes());
            }
        }
    }

    auto chDataIn = buffer.getArrayOfReadPointers();
    auto chDataOut = buffer.getArrayOfWritePointers();

    computeMpcAndHostOutputChannelIndicesToRender();

    server->work(chDataIn, chDataOut, buffer.getNumSamples(),
                 mpcMonoInputChannelIndices,
                 mpcMonoOutputChannelIndicesToRender, hostInputChannelIndices,
                 hostOutputChannelIndicesToRender);

    const bool shouldClearSomeHostChannels =
        !(previousHostOutputChannelIndicesToRender ==
          hostOutputChannelIndicesToRender);

    if (shouldClearSomeHostChannels)
    {
        for (int i = 0; i < totalNumOutputChannels; i++)
        {
            if (std::find(hostOutputChannelIndicesToRender.begin(),
                          hostOutputChannelIndicesToRender.end(),
                          i) == hostOutputChannelIndicesToRender.end())
            {
                buffer.clear(i, 0, buffer.getNumSamples());
            }
        }
    }

    // I've observed a crash in Ableton Live VST3 indicating what could be
    // allocating MIDI events too soon. So we give it a little bit of leeway of
    // 10000 frames.
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
        for (int i = lastHostChannelIndexThatWillBeWritten + 1;
             i < buffer.getNumChannels(); i++)
        {
            buffer.clear(i, 0, buffer.getNumSamples());
        }
    }

    previousHostOutputChannelIndicesToRender = hostOutputChannelIndicesToRender;
}

bool VmpcProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor *VmpcProcessor::createEditor()
{
    mpc.getLayeredScreen()->setDirty();
    return new VmpcEditor(*this);
}

void VmpcProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // Build XML for UI layout
    juce::XmlElement root("root");
    juce::XmlElement *juce_ui = root.createNewChildElement("JUCE-UI");

    if (auto *editor = getActiveEditor())
    {
        juce_ui->setAttribute("vector_ui_width", editor->getWidth());
        juce_ui->setAttribute("vector_ui_height", editor->getHeight());
    }

    // Standalone: only save window info
    if (juce::JUCEApplication::isStandaloneApp())
    {
        copyXmlToBinary(root, destData);
        return;
    }

    // Plugin: zip together UI XML + autosave
    juce::ZipFile::Builder builder;

    // --- 1. Add XML (HEAP allocation!) ---
    {
        juce::MemoryOutputStream xmlOut;
        root.writeTo(xmlOut, {});
        auto *xmlIn = new juce::MemoryInputStream(xmlOut.getData(),
                                                  xmlOut.getDataSize(), false);
        builder.addEntry(xmlIn, 0, "ui.xml", juce::Time::getCurrentTime());
    }

    // --- 2. Add autosave payload (HEAP allocation!) ---
    {
        auto mpcTarget = std::make_shared<ZipSaveTarget>();
        mpc::AutoSave::storeAutoSavedStateWithTarget(mpc, mpcTarget);
        auto zipBlock = mpcTarget->toZipMemoryBlock();

        auto *mpcIn = new juce::MemoryInputStream(zipBlock->getData(),
                                                  zipBlock->getSize(), false);
        builder.addEntry(mpcIn, 0, "autosave.zip",
                         juce::Time::getCurrentTime());
    }

    // --- 3. Finalize the ZIP ---
    juce::MemoryOutputStream combinedOut(destData, false);
    builder.writeToStream(combinedOut, nullptr);
}

void VmpcProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    if (sizeInBytes <= 0 || !data)
    {
        return;
    }

    // Standalone: restore only UI layout
    if (juce::JUCEApplication::isStandaloneApp())
    {
        std::unique_ptr<juce::XmlElement> xmlState(
            getXmlFromBinary(data, sizeInBytes));
        if (auto *juce_ui = xmlState->getChildByName("JUCE-UI"))
        {
            lastUIWidth =
                juce_ui->getIntAttribute("vector_ui_width", lastUIWidth);
            lastUIHeight =
                juce_ui->getIntAttribute("vector_ui_height", lastUIHeight);
        }
        return;
    }

    // Plugin: unpack combined zip
    juce::MemoryInputStream in(data, (size_t)sizeInBytes, false);
    juce::ZipFile zip(in);

    std::unique_ptr<juce::InputStream> uiStream, mpcStream;
    for (int i = 0; i < zip.getNumEntries(); ++i)
    {
        auto *entry = zip.getEntry(i);
        if (!entry)
        {
            continue;
        }

        if (entry->filename == "ui.xml")
        {
            uiStream.reset(zip.createStreamForEntry(i));
        }
        else if (entry->filename == "autosave.zip")
        {
            mpcStream.reset(zip.createStreamForEntry(i));
        }
    }

    // Restore UI dimensions
    if (uiStream)
    {
        auto xmlText = uiStream->readEntireStreamAsString();
        std::unique_ptr<juce::XmlElement> xmlState(juce::parseXML(xmlText));
        if (xmlState)
        {
            if (auto *juce_ui = xmlState->getChildByName("JUCE-UI"))
            {
                lastUIWidth =
                    juce_ui->getIntAttribute("vector_ui_width", lastUIWidth);
                lastUIHeight =
                    juce_ui->getIntAttribute("vector_ui_height", lastUIHeight);
            }
        }
    }

    // Restore MPC autosave
    if (mpcStream)
    {
        juce::MemoryOutputStream tempOut;
        tempOut.writeFromInputStream(*mpcStream, -1);
        auto block = tempOut.getMemoryBlock();

        auto zipTarget =
            std::make_shared<ZipSaveTarget>(block.getData(), block.getSize());
        mpc::AutoSave::restoreAutoSavedStateWithTarget(mpc, zipTarget);
    }
}

void VmpcProcessor::logActualBusLayout()
{
    MLOG("======= Actual input bus layout =======");
    for (int i = 0; i < getBusCount(true); i++)
    {
        const auto bus = getBus(true, i);
        MLOG("Bus " + std::to_string(i) + " (" + bus->getName().toStdString() +
             "):");
        MLOG("==              Enabled: " + std::to_string(bus->isEnabled()));
        if (bus->isMain() && i != 0)
        {
            MLOG("== !!!!!        Is main: " + std::to_string(bus->isMain()));
        }
        if (bus->getBusIndex() != i)
        {
            MLOG("== !!!!! Reported index: " +
                 std::to_string(bus->getBusIndex()));
        }
        MLOG("==  Last enabled layout: " +
             std::to_string(bus->getLastEnabledLayout().size()));
        MLOG("==       Current layout: " +
             std::to_string(bus->getCurrentLayout().size()));
        // MLOG("==       Default layout: " +
        // std::to_string(bus->getDefaultLayout().size()));
        std::string channelIndices;
        for (int c = 0; c < bus->getCurrentLayout().size(); c++)
        {
            channelIndices.append(
                std::to_string(bus->getChannelIndexInProcessBlockBuffer(c)) +
                ", ");
        }
        if (channelIndices.size() >= 2)
        {
            channelIndices =
                channelIndices.substr(0, channelIndices.size() - 2);
        }
        if (bus->getCurrentLayout().size() > 0)
        {
            MLOG("== Host channel indices: " + channelIndices);
        }
    }
    MLOG("======= End of actual input bus layout =======");
    MLOG("======= Actual output bus layout =======");
    for (int i = 0; i < getBusCount(false); i++)
    {
        const auto bus = getBus(false, i);
        MLOG("Bus " + std::to_string(i) + " (" + bus->getName().toStdString() +
             "):");
        MLOG("==              Enabled: " + std::to_string(bus->isEnabled()));
        if (bus->isMain() && i != 0)
        {
            MLOG("== !!!!!        Is main: " + std::to_string(bus->isMain()));
        }
        if (bus->getBusIndex() != i)
        {
            MLOG("== !!!!! Reported index: " +
                 std::to_string(bus->getBusIndex()));
        }
        MLOG("==  Last enabled layout: " +
             std::to_string(bus->getLastEnabledLayout().size()));
        MLOG("==       Current layout: " +
             std::to_string(bus->getCurrentLayout().size()));
        // MLOG("==       Default layout: " +
        // std::to_string(bus->getDefaultLayout().size()));
        std::string channelIndices;
        for (int c = 0; c < bus->getCurrentLayout().size(); c++)
        {
            channelIndices.append(
                std::to_string(bus->getChannelIndexInProcessBlockBuffer(c)) +
                ", ");
        }
        if (channelIndices.size() >= 2)
        {
            channelIndices =
                channelIndices.substr(0, channelIndices.size() - 2);
        }
        MLOG("== Host channel indices: " + channelIndices);
    }
    MLOG("======= End of actual output bus layout =======");
}

void VmpcProcessor::computeHostToMpcChannelMappings()
{
    mpcMonoInputChannelIndices.clear();
    mpcMonoOutputChannelIndices.clear();
    hostInputChannelIndices.clear();
    hostOutputChannelIndices.clear();
    lastHostChannelIndexThatWillBeWritten = 0;

    if (juce::JUCEApplication::isStandaloneApp())
    {
        assert(getBusCount(false) == 1);

        for (int i = 0; i < getBus(false, 0)->getNumberOfChannels(); i++)
        {
            if (i > 9)
            {
                break;
            }

            mpcMonoOutputChannelIndices.push_back(static_cast<int8_t>(i));
            hostOutputChannelIndices.push_back(static_cast<int8_t>(
                getBus(false, 0)->getChannelIndexInProcessBlockBuffer(i)));
            lastHostChannelIndexThatWillBeWritten =
                std::max<int8_t>(lastHostChannelIndexThatWillBeWritten,
                                 hostOutputChannelIndices.back());
        }

        for (int i = 0; i < getBus(true, 0)->getNumberOfChannels(); i++)
        {
            if (i > 1)
            {
                break;
            }

            mpcMonoInputChannelIndices.push_back(static_cast<int8_t>(i));
            hostInputChannelIndices.push_back(static_cast<int8_t>(
                getBus(true, 0)->getChannelIndexInProcessBlockBuffer(i)));
        }

        if (mpcMonoInputChannelIndices.size() == 1 &&
            hostInputChannelIndices.size() == 1)
        {
            mpcMonoInputChannelIndices.push_back(
                mpcMonoInputChannelIndices.back() + 1);
            hostInputChannelIndices.push_back(hostInputChannelIndices.back());
        }

        return;
    }

    int mpcMonoChannelCounter = 0;

    for (int i = 0; i < getBusCount(true); i++)
    {
        if (mpcMonoChannelCounter >= 2)
        {
            mpcMonoChannelCounter -= 2;
        }

        const auto bus = getBus(true, i);
        const bool busWasRequestedToBeStereoButIsMono =
            bus->getBusIndex() <= 4 && bus->getLastEnabledLayout().size() == 1;
        const bool busWasRequestedToBeMonoButIsStereo =
            bus->getBusIndex() >= 5 && bus->getLastEnabledLayout().size() == 2;

        int mpcMonoChannelsToAdd = bus->getLastEnabledLayout().size();

        if (busWasRequestedToBeMonoButIsStereo)
        {
            mpcMonoChannelsToAdd = 1;
        }
        else if (busWasRequestedToBeStereoButIsMono)
        {
            mpcMonoChannelsToAdd = 2;
        }

        if (!bus->isEnabled())
        {
            mpcMonoChannelCounter += mpcMonoChannelsToAdd;
            continue;
        }

        if (bus->getBusIndex() < 1)
        {
            const bool busWasRequestedToBeStereoButIsMono2 =
                bus->getNumberOfChannels() == 1;

            hostInputChannelIndices.push_back(static_cast<int8_t>(
                bus->getChannelIndexInProcessBlockBuffer(0)));
            mpcMonoInputChannelIndices.push_back(
                static_cast<int8_t>(mpcMonoChannelCounter));

            hostInputChannelIndices.push_back(
                static_cast<int8_t>(bus->getChannelIndexInProcessBlockBuffer(
                    busWasRequestedToBeStereoButIsMono2 ? 0 : 1)));
            mpcMonoInputChannelIndices.push_back(
                static_cast<int8_t>(mpcMonoChannelCounter + 1));
        }
        else
        {
            jassert(bus->getNumberOfChannels() == 1);
            hostInputChannelIndices.push_back(static_cast<int8_t>(
                bus->getChannelIndexInProcessBlockBuffer(0)));
            mpcMonoInputChannelIndices.push_back(
                static_cast<int8_t>(mpcMonoChannelCounter));
        }

        mpcMonoChannelCounter += mpcMonoChannelsToAdd;
    }

    mpcMonoChannelCounter = 0;

    for (int i = 0; i < getBusCount(false); i++)
    {
        if (mpcMonoChannelCounter >= 10)
        {
            mpcMonoChannelCounter -= 10;
        }

        const auto bus = getBus(false, i);

        /*
         * In some cases we specify a bus to be mono, but we still get a stereo
         * bus. This has happened to me most notably in Ableton Live, because it
         * does not have true mono buses. So we keep track of such cases, and
         * compute our channel mapping accordingly.
         */

        const bool busWasRequestedToBeStereoButIsMono =
            bus->getBusIndex() <= 4 && bus->getLastEnabledLayout().size() == 1;
        const bool busWasRequestedToBeMonoButIsStereo =
            bus->getBusIndex() >= 5 && bus->getLastEnabledLayout().size() == 2;

        int mpcMonoChannelsToAdd = bus->getLastEnabledLayout().size();

        if (busWasRequestedToBeMonoButIsStereo)
        {
            mpcMonoChannelsToAdd = 1;
        }
        else if (busWasRequestedToBeStereoButIsMono)
        {
            mpcMonoChannelsToAdd = 2;
        }

        if (!bus->isEnabled())
        {
            // MLOG("Bus " + bus->getName().toStdString() + " with index " +
            // std::to_string(bus->getBusIndex()) + " is not enabled, adding " +
            // std::to_string(mpcMonoChannelsToAdd) + " channels");
            mpcMonoChannelCounter += mpcMonoChannelsToAdd;
            continue;
        }

        if (bus->getBusIndex() < 5)
        {
            // Here we process stereo buses STEREO OUT L/R, MIX 1/2, MIX 3/4,
            // MIX 5/6 and MIX 7/8
            hostOutputChannelIndices.push_back(static_cast<int8_t>(
                bus->getChannelIndexInProcessBlockBuffer(0)));
            mpcMonoOutputChannelIndices.push_back(
                static_cast<int8_t>(mpcMonoChannelCounter));

            if (busWasRequestedToBeStereoButIsMono)
            {
                lastHostChannelIndexThatWillBeWritten = std::max<int8_t>(
                    lastHostChannelIndexThatWillBeWritten,
                    static_cast<int8_t>(
                        bus->getChannelIndexInProcessBlockBuffer(0)));
            }
            else
            {
                hostOutputChannelIndices.push_back(static_cast<int8_t>(
                    bus->getChannelIndexInProcessBlockBuffer(1)));
                mpcMonoOutputChannelIndices.push_back(
                    static_cast<int8_t>(mpcMonoChannelCounter + 1));
                lastHostChannelIndexThatWillBeWritten = std::max<int8_t>(
                    lastHostChannelIndexThatWillBeWritten,
                    static_cast<int8_t>(
                        bus->getChannelIndexInProcessBlockBuffer(1)));
            }
        }
        else
        {
            // Here we process mono buses STEREO OUT L, STEREO OUT R, MIX 1 ...
            // MIX 8
            hostOutputChannelIndices.push_back(static_cast<int8_t>(
                bus->getChannelIndexInProcessBlockBuffer(0)));
            mpcMonoOutputChannelIndices.push_back(
                static_cast<int8_t>(mpcMonoChannelCounter + 2));

            if (busWasRequestedToBeMonoButIsStereo)
            {
                hostOutputChannelIndices.push_back(static_cast<int8_t>(
                    bus->getChannelIndexInProcessBlockBuffer(1)));
                mpcMonoOutputChannelIndices.push_back(
                    static_cast<int8_t>(mpcMonoChannelCounter + 2));
                lastHostChannelIndexThatWillBeWritten = std::max<int8_t>(
                    lastHostChannelIndexThatWillBeWritten,
                    static_cast<int8_t>(
                        bus->getChannelIndexInProcessBlockBuffer(1)));
            }
            else
            {
                lastHostChannelIndexThatWillBeWritten = std::max<int8_t>(
                    lastHostChannelIndexThatWillBeWritten,
                    static_cast<int8_t>(
                        bus->getChannelIndexInProcessBlockBuffer(0)));
            }
        }

        mpcMonoChannelCounter += mpcMonoChannelsToAdd;
    }

    /*
    MLOG("==============================================");
    MLOG("    Number of hostOutputChanelIndices: " +
    std::to_string(hostOutputChannelIndices.size())); MLOG("Number of
    mpcMonoOutputChannelIndices: " +
    std::to_string(mpcMonoOutputChannelIndices.size()));

    for (int i = 0; i < hostOutputChannelIndices.size(); i++)
    {
        MLOG("mpc channel " + std::to_string(mpcMonoOutputChannelIndices[i]) + "
    will be put in host channel " +
    std::to_string(hostOutputChannelIndices[i]));
    }
    MLOG("==============================================");
    */
}

void VmpcProcessor::computePossiblyActiveMpcMonoOutChannels()
{
    possiblyActiveMpcMonoOutChannels.clear();

    auto insertValue = [&](const int8_t value)
    {
        possiblyActiveMpcMonoOutChannels.insert(value);

        // It's not trivial to figure out if an MPC mixer strip is mono or
        // stereo, because it it depends on whether the strip is associated with
        // a mono or stereo sound. The main idea behind
        // `getPossiblyActiveMpcMonoOutChannels` is to avoid unnecessary
        // rendering of MIX busses in AUv2 and AUv3. The problem here is that,
        // so far, I'm not aware of a way to make an AUv2/3 expose 2 fixed bus
        // layouts to Logic, of which one has mixed mono and stereo channels:
        // 1. 1x stereo in, 1x stereo out
        // 2. 1x stereo in, 5x stereo out and 8x mono out
        // This has led to the decision to rely on "implicit" or "default"
        // layouts. See https://github.com/juce-framework/JUCE/issues/1508 This
        // in turn poses the problem that in Logic, all 13 busses are always
        // reported to be active, even if the user loads the 1x stereo in, 1x
        // stereo out plugin flavor. So it's fine if we assume all busses are
        // stereo, as this will still prevent rendering MIX1...8, as long as the
        // user has not configured to use individual outputs. That's why for
        // every mono channel, we always ensure its stereo counterpart is also
        // rendered. I.e. if we render channel 0, we also render channel 1 and
        // vice versa.
        if (value % 2 == 0)
        {
            possiblyActiveMpcMonoOutChannels.insert(value + 1);
        }
        else
        {
            possiblyActiveMpcMonoOutChannels.insert(value - 1);
        }
    };

    // We always render STEREO L/R
    insertValue(0);
    insertValue(1);

    for (auto &p : mpc.getSampler()->getPrograms())
    {
        if (!p.lock())
        {
            continue;
        }
        for (auto &n : p.lock()->getNotesParameters())
        {
            const auto output = n->getIndivFxMixerChannel()->getOutput();
            if (output == 0)
            {
                continue;
            }
            // output is 1 for MIX 1 bus, 2 for MIX 2 bus, etc. So we subtract 1
            // to get 0-based index, and then we add 2 to get the bus index in
            // the plugin, because the first stereo bus (for STEREO L/R, the
            // main output) comes before the MIX busses.
            insertValue(static_cast<int8_t>(output + 1));
        }
    }

    for (int i = 0; i < 4; i++)
    {
        for (auto &m : mpc.getDrum(i).getIndivFxMixerChannels())
        {
            if (m->getOutput() == 0)
            {
                continue;
            }
            insertValue(static_cast<int8_t>(m->getOutput() + 1));
        }
    }
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new VmpcProcessor();
}
