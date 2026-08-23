#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace vmpc_juce::gui::mobile
{
    class RecordingPreviewPlayer
    {
    public:
        RecordingPreviewPlayer();
        ~RecordingPreviewPlayer();

        void prepare(double sampleRate, int maximumBlockSize);
        void release();
        bool start(const juce::File &file);
        void stop();
        bool isPlaying() const;
        void render(juce::AudioBuffer<float> &destination);

    private:
        mutable juce::CriticalSection lock;
        juce::AudioFormatManager formatManager;
        juce::TimeSliceThread readAheadThread{"Recording preview"};
        juce::AudioTransportSource transport;
        std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
        juce::AudioBuffer<float> renderBuffer;
        bool prepared = false;
    };
} // namespace vmpc_juce::gui::mobile
