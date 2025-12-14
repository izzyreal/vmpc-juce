#include "VmpcStandalonePlayer.hpp"

#include <cassert>

using namespace vmpc_juce;

static void initialiseIoBuffers(const juce::Span<const float *const> ins,
                                const juce::Span<float *const> outs,
                                const int numSamples, const size_t processorIns,
                                const size_t processorOuts,
                                juce::AudioBuffer<float> &tempBuffer,
                                std::vector<float *> &channels)
{
    const auto totalNumChannels = juce::jmax(processorIns, processorOuts);

    assert(channels.capacity() >= totalNumChannels);
    assert(static_cast<size_t>(tempBuffer.getNumChannels()) >=
           totalNumChannels);
    assert(tempBuffer.getNumSamples() >= numSamples);

    channels.resize(totalNumChannels);

    const auto numBytes = static_cast<size_t>(numSamples) * sizeof(float);

    size_t tempBufferIndex = 0;

    for (size_t i = 0; i < totalNumChannels; ++i)
    {
        auto *&channelPtr = channels[i];
        channelPtr = i < outs.size() ? outs[i]
                                     : tempBuffer.getWritePointer(
                                           static_cast<int>(tempBufferIndex++));

        // If there's a single input channel, route it to all inputs on the
        // processor
        if (ins.size() == 1 && i < processorIns)
        {
            memcpy(channelPtr, ins.front(), numBytes);
        }

        // Otherwise, if there's a system input corresponding to this
        // channel, use that
        else if (i < ins.size())
        {
            memcpy(channelPtr, ins[i], numBytes);
        }

        // Otherwise, silence the channel
        else
        {
            juce::zeromem(channelPtr, numBytes);
        }
    }

    // Zero any output channels that won't be written by the processor
    for (size_t i = totalNumChannels; i < outs.size(); ++i)
    {
        juce::zeromem(outs[i], numBytes);
    }
}

//==============================================================================
VmpcStandalonePlayer::VmpcStandalonePlayer(
    const bool doDoublePrecisionProcessing)
    : isDoublePrecision(doDoublePrecisionProcessing), currentWorkgroup()
{
    incomingMidi.ensureSize(8192);
    messageCollector.ensureStorageAllocated(8192);
}

VmpcStandalonePlayer::~VmpcStandalonePlayer()
{
    setProcessor(nullptr);
}

VmpcStandalonePlayer::NumChannels VmpcStandalonePlayer::findMostSuitableLayout(
    const juce::AudioProcessor &proc) const
{
    if (proc.isMidiEffect())
    {
        return {};
    }

    std::vector layouts{deviceChannels};

    if (deviceChannels.ins == 0 || deviceChannels.ins == 1)
    {
        layouts.emplace_back(defaultProcessorChannels.ins, deviceChannels.outs);
        layouts.emplace_back(deviceChannels.outs, deviceChannels.outs);
    }

    const auto it = std::find_if(layouts.begin(), layouts.end(),
                                 [&](const NumChannels &chans)
                                 {
                                     return proc.checkBusesLayoutSupported(
                                         chans.toLayout());
                                 });

    if (it == layouts.end())
    {
        return defaultProcessorChannels;
    }

    return *it;
}

void VmpcStandalonePlayer::resizeChannels()
{
    const auto maxChannels =
        juce::jmax(deviceChannels.ins, deviceChannels.outs,
                   actualProcessorChannels.ins, actualProcessorChannels.outs);
    channels.resize(static_cast<size_t>(maxChannels));
    tempBuffer.setSize(maxChannels, blockSize);
}

void VmpcStandalonePlayer::setProcessor(
    juce::AudioProcessor *const processorToPlay)
{
    const juce::ScopedLock sl(lock);

    if (processor == processorToPlay)
    {
        return;
    }

    sampleCount = 0;
    currentWorkgroup.reset();

    if (processorToPlay != nullptr && sampleRate > 0 && blockSize > 0)
    {
        defaultProcessorChannels =
            NumChannels{processorToPlay->getBusesLayout()};
        actualProcessorChannels = findMostSuitableLayout(*processorToPlay);

        if (processorToPlay->isMidiEffect())
        {
            processorToPlay->setRateAndBufferSizeDetails(sampleRate, blockSize);
        }
        else
        {
            processorToPlay->setPlayConfigDetails(actualProcessorChannels.ins,
                                                  actualProcessorChannels.outs,
                                                  sampleRate, blockSize);
        }

        const auto supportsDouble =
            processorToPlay->supportsDoublePrecisionProcessing() &&
            isDoublePrecision;

        processorToPlay->setProcessingPrecision(
            supportsDouble ? juce::AudioProcessor::doublePrecision
                           : juce::AudioProcessor::singlePrecision);

        processorToPlay->prepareToPlay(sampleRate, blockSize);
    }

    juce::AudioProcessor *oldOne = nullptr;

    oldOne = isPrepared ? processor : nullptr;
    processor = processorToPlay;
    isPrepared = true;
    resizeChannels();

    if (oldOne != nullptr)
    {
        oldOne->releaseResources();
    }
}

void VmpcStandalonePlayer::setDoublePrecisionProcessing(
    const bool doublePrecision)
{
    if (doublePrecision != isDoublePrecision)
    {
        const juce::ScopedLock sl(lock);

        currentWorkgroup.reset();

        if (processor != nullptr)
        {
            processor->releaseResources();

            const auto supportsDouble =
                processor->supportsDoublePrecisionProcessing() &&
                doublePrecision;

            processor->setProcessingPrecision(
                supportsDouble ? juce::AudioProcessor::doublePrecision
                               : juce::AudioProcessor::singlePrecision);

            processor->prepareToPlay(sampleRate, blockSize);
        }

        isDoublePrecision = doublePrecision;
    }
}

void VmpcStandalonePlayer::setMidiOutput(juce::MidiOutput *midiOutputToUse)
{
    if (midiOutput != midiOutputToUse)
    {
        const juce::ScopedLock sl(lock);

        if (midiOutput != nullptr)
        {
            midiOutput->stopBackgroundThread();
        }

        midiOutput = midiOutputToUse;

        if (midiOutput != nullptr)
        {
            midiOutput->startBackgroundThread();
        }
    }
}

//==============================================================================
void VmpcStandalonePlayer::audioDeviceIOCallbackWithContext(
    const float *const *const inputChannelData, const int numInputChannels,
    float *const *const outputChannelData, const int numOutputChannels,
    const int numSamples, const juce::AudioIODeviceCallbackContext &context)
{
    const juce::ScopedLock sl(lock);

    assert(currentDevice != nullptr);

    // These should have been prepared by audioDeviceAboutToStart()...
    assert(sampleRate > 0 && blockSize > 0);

    incomingMidi.clear();
    messageCollector.removeNextBlockOfMessages(incomingMidi, numSamples);

    initialiseIoBuffers(
        {inputChannelData, static_cast<size_t>(numInputChannels)},
        {outputChannelData, static_cast<size_t>(numOutputChannels)}, numSamples,
        static_cast<size_t>(actualProcessorChannels.ins),
        static_cast<size_t>(actualProcessorChannels.outs), tempBuffer,
        channels);

    const auto totalNumChannels =
        juce::jmax(actualProcessorChannels.ins, actualProcessorChannels.outs);
    juce::AudioBuffer buffer(channels.data(), totalNumChannels, numSamples);

    if (processor != nullptr)
    {
        const juce::ScopedLock sl2(processor->getCallbackLock());

        if (std::exchange(currentWorkgroup, currentDevice->getWorkgroup()) !=
            currentDevice->getWorkgroup())
        {
            processor->audioWorkgroupContextChanged(currentWorkgroup);
        }

        class PlayHead final : juce::AudioPlayHead
        {
        public:
            PlayHead(juce::AudioProcessor &proc,
                     const juce::Optional<uint64_t> hostTimeIn,
                     const uint64_t sampleCountIn, const double sampleRateIn)
                : processor(proc), hostTimeNs(hostTimeIn),
                  sampleCount(sampleCountIn),
                  seconds(static_cast<double>(sampleCountIn) / sampleRateIn)
            {
                if (useThisPlayhead)
                {
                    processor.setPlayHead(this);
                }
            }

            ~PlayHead() override
            {
                if (useThisPlayhead)
                {
                    processor.setPlayHead(nullptr);
                }
            }

        private:
            juce::Optional<PositionInfo> getPosition() const override
            {
                PositionInfo info;
                info.setHostTimeNs(hostTimeNs);
                info.setTimeInSamples(static_cast<int64_t>(sampleCount));
                info.setTimeInSeconds(seconds);
                return info;
            }

            juce::AudioProcessor &processor;
            juce::Optional<uint64_t> hostTimeNs;
            uint64_t sampleCount;
            double seconds;
            bool useThisPlayhead = processor.getPlayHead() == nullptr;
        };

        PlayHead playHead{*processor,
                          context.hostTimeNs != nullptr
                              ? juce::makeOptional(*context.hostTimeNs)
                              : juce::nullopt,
                          sampleCount, sampleRate};

        sampleCount += static_cast<uint64_t>(numSamples);

        if (!processor->isSuspended())
        {
            if (processor->isUsingDoublePrecision())
            {
                conversionBuffer.makeCopyOf(buffer, true);
                processor->processBlock(conversionBuffer, incomingMidi);
                buffer.makeCopyOf(conversionBuffer, true);
            }
            else
            {
                processor->processBlock(buffer, incomingMidi);
            }

            if (midiOutput != nullptr)
            {
                if (midiOutput->isBackgroundThreadRunning())
                {
                    midiOutput->sendBlockOfMessages(
                        incomingMidi, juce::Time::getMillisecondCounterHiRes(),
                        sampleRate);
                }
                else
                {
                    midiOutput->sendBlockOfMessagesNow(incomingMidi);
                }
            }

            return;
        }
    }

    for (int i = 0; i < numOutputChannels; ++i)
    {
        juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
    }
}

void VmpcStandalonePlayer::audioDeviceAboutToStart(
    juce::AudioIODevice *const device)
{
    currentDevice = device;
    const auto newSampleRate = device->getCurrentSampleRate();
    const auto newBlockSize = device->getCurrentBufferSizeSamples();
    auto numChansIn = device->getActiveInputChannels().countNumberOfSetBits();
    auto numChansOut = device->getActiveOutputChannels().countNumberOfSetBits();

    const juce::ScopedLock sl(lock);

    sampleRate = newSampleRate;
    blockSize = newBlockSize;
    deviceChannels = {numChansIn, numChansOut};

    resizeChannels();

    messageCollector.reset(sampleRate);

    currentWorkgroup.reset();

    if (processor != nullptr)
    {
        if (isPrepared)
        {
            processor->releaseResources();
        }

        auto *oldProcessor = processor;
        setProcessor(nullptr);
        setProcessor(oldProcessor);
    }
}

void VmpcStandalonePlayer::audioDeviceStopped()
{
    const juce::ScopedLock sl(lock);

    if (processor != nullptr && isPrepared)
    {
        processor->releaseResources();
    }

    sampleRate = 0.0;
    blockSize = 0;
    isPrepared = false;
    tempBuffer.setSize(1, 1);

    currentDevice = nullptr;
    currentWorkgroup.reset();
}

void VmpcStandalonePlayer::handleIncomingMidiMessage(
    juce::MidiInput *, const juce::MidiMessage &message)
{
    messageCollector.addMessageToQueue(message);
}
