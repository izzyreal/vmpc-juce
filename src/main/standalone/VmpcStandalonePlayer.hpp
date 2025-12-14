#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace vmpc_juce::standalone
{
    class VmpcStandalonePlayer final : public juce::AudioIODeviceCallback,
                                       public juce::MidiInputCallback
    {
    public:
        explicit VmpcStandalonePlayer(bool doDoublePrecisionProcessing = false);

        ~VmpcStandalonePlayer() override;

        void setProcessor(juce::AudioProcessor *processorToPlay);

        juce::AudioProcessor *getCurrentProcessor() const noexcept
        {
            return processor;
        }

        juce::MidiMessageCollector &getMidiMessageCollector() noexcept
        {
            return messageCollector;
        }

        void setMidiOutput(juce::MidiOutput *midiOutputToUse);

        void setDoublePrecisionProcessing(bool doublePrecision);

        bool getDoublePrecisionProcessing() const
        {
            return isDoublePrecision;
        }

        void audioDeviceIOCallbackWithContext(
            const float *const *, int, float *const *, int, int,
            const juce::AudioIODeviceCallbackContext &) override;

        void audioDeviceAboutToStart(juce::AudioIODevice *) override;

        void audioDeviceStopped() override;

        void handleIncomingMidiMessage(juce::MidiInput *,
                                       const juce::MidiMessage &) override;

    private:
        struct NumChannels
        {
            NumChannels() = default;
            NumChannels(const int numIns, const int numOuts)
                : ins(numIns), outs(numOuts)
            {
            }

            explicit NumChannels(
                const juce::AudioProcessor::BusesLayout &layout)
                : ins(layout.getNumChannels(true, 0)),
                  outs(layout.getNumChannels(false, 0))
            {
            }

            juce::AudioProcessor::BusesLayout toLayout() const
            {
                return {{juce::AudioChannelSet::canonicalChannelSet(ins)},
                        {juce::AudioChannelSet::canonicalChannelSet(outs)}};
            }

            int ins = 0, outs = 0;
        };

        NumChannels findMostSuitableLayout(const juce::AudioProcessor &) const;
        void resizeChannels();

        juce::AudioProcessor *processor = nullptr;
        juce::CriticalSection lock;
        double sampleRate = 0;
        int blockSize = 0;
        bool isPrepared = false, isDoublePrecision = false;

        NumChannels deviceChannels, defaultProcessorChannels,
            actualProcessorChannels;
        std::vector<float *> channels;
        juce::AudioBuffer<float> tempBuffer;
        juce::AudioBuffer<double> conversionBuffer;

        juce::MidiBuffer incomingMidi;
        juce::MidiMessageCollector messageCollector;
        juce::MidiOutput *midiOutput = nullptr;
        uint64_t sampleCount = 0;

        juce::AudioIODevice *currentDevice = nullptr;
        juce::AudioWorkgroup currentWorkgroup;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VmpcStandalonePlayer)
    };

} // namespace vmpc_juce::standalone