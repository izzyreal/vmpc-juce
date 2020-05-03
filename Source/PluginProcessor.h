/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <Mpc.hpp>

//==============================================================================
/**
*/
class VmpcAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    VmpcAudioProcessor();
    ~VmpcAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

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
	void processMidiOut(MidiBuffer& midiMessages, int bufferSize);
	void processTransport();
    void checkBouncing();
    void checkSoundRecorder();

private:
    AudioSampleBuffer monoToStereoBuffer;
	double m_Tempo = 0;
    bool wasPlaying = false;
    bool wasBouncing = false;
    bool wasRecordingSound = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcAudioProcessor)
};
