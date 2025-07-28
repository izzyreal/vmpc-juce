#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <Mpc.hpp>

namespace mpc::engine::midi { class ShortMessage; }

namespace vmpc_juce {

template <typename T, std::size_t Capacity>
class FixedSet {
    std::array<T, Capacity> data_;
    std::size_t size_ = 0;

public:
    void insert(const T& value) {
        for (std::size_t i = 0; i < size_; ++i)
            if (data_[i] == value) return;
        if (size_ < Capacity) {
            data_[size_++] = value;
        }
    }

    bool contains(const T& value) const {
        for (std::size_t i = 0; i < size_; ++i)
            if (data_[i] == value) return true;
        return false;
    }

    void clear()
    {
        size_ = 0;
    }
};

class VmpcProcessor  : public juce::AudioProcessor {

    public:
        VmpcProcessor();
        ~VmpcProcessor() override;

        void prepareToPlay (double sampleRate, int samplesPerBlock) override;

        void releaseResources() override {}

        bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

        void processBlock (juce::AudioSampleBuffer&, juce::MidiBuffer&) override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override;

        const juce::String getName() const override;

        bool acceptsMidi() const override;
        bool producesMidi() const override;
        bool isMidiEffect () const override;
        double getTailLengthSeconds() const override;

        int getNumPrograms() override;
        int getCurrentProgram() override;
        void setCurrentProgram (int index) override;
        const juce::String getProgramName (int index) override;
        void changeProgramName (int index, const juce::String& newName) override;

        void getStateInformation (juce::MemoryBlock& destData) override;
        void setStateInformation (const void* data, int sizeInBytes) override;

        int lastUIWidth = 0, lastUIHeight = 0;

    private:
        void processMidiIn(juce::MidiBuffer& midiMessages);
        void processMidiOut(juce::MidiBuffer& midiMessages, bool discard);
        void processTransport();
        void computeHostToMpcChannelMappings();

        std::vector<uint8_t> mpcMonoInputChannelIndices, mpcMonoOutputChannelIndices, hostInputChannelIndices, hostOutputChannelIndices;
        std::vector<uint8_t> previousHostOutputChannelIndicesToRender;
        uint8_t lastHostChannelIndexThatWillBeWritten = 0;
        double m_Tempo = 0;
        bool wasPlaying = false;
        int framesProcessed = 0;

        std::vector<std::shared_ptr<mpc::engine::midi::ShortMessage>> midiOutputBuffer =
            std::vector<std::shared_ptr<mpc::engine::midi::ShortMessage>>(100);

        static BusesProperties getBusesProperties();

        bool layoutChanged = false;

        FixedSet<uint8_t, 10> possiblyActiveMpcMonoOutChannels;

        void computePossiblyActiveMpcMonoOutChannels();

        void logActualBusLayout();

    public:
        bool shouldShowDisclaimer = true;
        std::function<void()> showAudioSettingsDialog = [](){};
        mpc::Mpc mpc;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcProcessor)
};
}
