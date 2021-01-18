#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <Mpc.hpp>

using namespace juce;

class VmpcAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    VmpcAudioProcessor();
    ~VmpcAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect () const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	int lastUIWidth = 1298/2, lastUIHeight = 994/2;

private:
	void processMidiIn(MidiBuffer& midiMessages);
	void processMidiOut(MidiBuffer& midiMessages);
	void processTransport();
    void checkBouncing();
    void checkSoundRecorder();

private:
    AudioSampleBuffer monoToStereoBuffer;
	double m_Tempo = 0;
    bool wasPlaying = false;
    bool wasBouncing = false;
    bool wasRecordingSound = false;

public:
    bool shouldShowDisclaimer = true;
    bool poweredUp = false;
    mpc::Mpc mpc;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcAudioProcessor)
};
