#pragma once

#include "client/event/ClientMidiEvent.hpp"

#include <juce_audio_processors/juce_audio_processors.h>

#include <Mpc.hpp>

#include <limits>
#include <vector>
#include <unordered_set>

namespace vmpc_juce
{
    class VmpcProcessor final : public juce::AudioProcessor
    {
    public:
        VmpcProcessor();
        ~VmpcProcessor() override;

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;

        void releaseResources() override {}

        bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

        void processBlock(juce::AudioSampleBuffer &,
                          juce::MidiBuffer &) override;

        juce::AudioProcessorEditor *createEditor() override;
        bool hasEditor() const override;

        const juce::String getName() const override;

        bool acceptsMidi() const override;
        bool producesMidi() const override;
        bool isMidiEffect() const override;
        double getTailLengthSeconds() const override;

        int getNumPrograms() override;
        int getCurrentProgram() override;
        void setCurrentProgram(int index) override;
        const juce::String getProgramName(int index) override;
        void changeProgramName(int index, const juce::String &newName) override;

        void getStateInformation(juce::MemoryBlock &destData) override;
        void setStateInformation(const void *data, int sizeInBytes) override;

        int lastUIWidth = 0, lastUIHeight = 0;

    private:
        void processMidiIn(const juce::MidiBuffer &midiMessages) const;
        void processMidiOut(juce::MidiBuffer &midiMessages, bool discard);
        void processTransport();
        void computeHostToMpcChannelMappings();

        std::vector<int8_t> mpcMonoInputChannelIndices,
            mpcMonoOutputChannelIndices, hostInputChannelIndices,
            hostOutputChannelIndices;

        std::vector<int8_t> mpcMonoOutputChannelIndicesToRender;
        std::vector<int8_t> hostOutputChannelIndicesToRender;
        std::vector<int8_t> previousHostOutputChannelIndicesToRender;

        void computeMpcAndHostOutputChannelIndicesToRender();

        int8_t lastHostChannelIndexThatWillBeWritten = 0;
        double m_Tempo = 0;
        bool wasPlaying = false;
        int framesProcessed = 0;
        double previousPositionQuarterNotes =
            std::numeric_limits<double>::lowest();

        static BusesProperties getBusesProperties();

        std::unordered_set<int8_t> possiblyActiveMpcMonoOutChannels;

        void computePossiblyActiveMpcMonoOutChannels();

        void logActualBusLayout();

        std::vector<mpc::client::event::ClientMidiEvent> midiOutputBuffer;

    public:
        bool shouldShowDisclaimer = true;
        std::function<void()> showAudioSettingsDialog = []() {};
        mpc::Mpc mpc;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VmpcProcessor)
    };
} // namespace vmpc_juce
