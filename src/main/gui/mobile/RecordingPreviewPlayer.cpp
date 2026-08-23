#include "gui/mobile/RecordingPreviewPlayer.hpp"

using namespace vmpc_juce::gui::mobile;

RecordingPreviewPlayer::RecordingPreviewPlayer()
{
    formatManager.registerBasicFormats();
    readAheadThread.startThread();
}

RecordingPreviewPlayer::~RecordingPreviewPlayer()
{
    release();
    readAheadThread.stopThread(2000);
}

void RecordingPreviewPlayer::prepare(const double sampleRate,
                                     const int maximumBlockSize)
{
    const juce::ScopedLock scopedLock(lock);
    if (prepared)
    {
        transport.releaseResources();
    }
    transport.prepareToPlay(maximumBlockSize, sampleRate);
    renderBuffer.setSize(2, juce::jmax(1, maximumBlockSize), false, false,
                         true);
    prepared = true;
}

void RecordingPreviewPlayer::release()
{
    const juce::ScopedLock scopedLock(lock);
    transport.stop();
    transport.setSource(nullptr);
    readerSource.reset();
    if (prepared)
    {
        transport.releaseResources();
        prepared = false;
    }
}

bool RecordingPreviewPlayer::start(const juce::File &file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));
    if (reader == nullptr)
    {
        return false;
    }

    const auto sourceSampleRate = reader->sampleRate;
    auto newSource =
        std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);

    const juce::ScopedLock scopedLock(lock);
    transport.stop();
    transport.setSource(nullptr);
    readerSource = std::move(newSource);
    transport.setSource(readerSource.get(), 32768, &readAheadThread,
                        sourceSampleRate, 2);
    transport.setPosition(0.0);
    transport.start();
    return true;
}

void RecordingPreviewPlayer::stop()
{
    const juce::ScopedLock scopedLock(lock);
    transport.stop();
    transport.setPosition(0.0);
}

bool RecordingPreviewPlayer::isPlaying() const
{
    const juce::ScopedLock scopedLock(lock);
    return transport.isPlaying();
}

void RecordingPreviewPlayer::render(juce::AudioBuffer<float> &destination)
{
    if (destination.getNumChannels() == 0 || destination.getNumSamples() == 0)
    {
        return;
    }

    const juce::ScopedTryLock scopedLock(lock);
    if (!scopedLock.isLocked())
    {
        return;
    }
    if (!transport.isPlaying())
    {
        return;
    }

    renderBuffer.setSize(2, destination.getNumSamples(), false, false, true);
    renderBuffer.clear();
    juce::AudioSourceChannelInfo info(&renderBuffer, 0,
                                      destination.getNumSamples());
    transport.getNextAudioBlock(info);

    const auto channelsToMix = juce::jmin(2, destination.getNumChannels());
    for (int channel = 0; channel < channelsToMix; ++channel)
    {
        destination.addFrom(channel, 0, renderBuffer, channel, 0,
                            destination.getNumSamples());
    }
}
