#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <Mpc.hpp>

namespace mpc::engine::midi { class ShortMessage; }

class VmpcAudioProcessor  : public juce::AudioProcessor
{
public:
  //==============================================================================
  VmpcAudioProcessor();
  ~VmpcAudioProcessor() override;

  //==============================================================================
  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  
  bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
  
  void processBlock (juce::AudioSampleBuffer&, juce::MidiBuffer&) override;
  
  //==============================================================================
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;
  
  //==============================================================================
  const juce::String getName() const override;
  
  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect () const override;
  double getTailLengthSeconds() const override;
  
  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram (int index) override;
  const juce::String getProgramName (int index) override;
  void changeProgramName (int index, const juce::String& newName) override;
  
  //==============================================================================
  void getStateInformation (juce::MemoryBlock& destData) override;
  void setStateInformation (const void* data, int sizeInBytes) override;
  
  int lastUIWidth = 1298/2, lastUIHeight = 994/2;
  
private:
  void processMidiIn(juce::MidiBuffer& midiMessages);
  void processMidiOut(juce::MidiBuffer& midiMessages);
  void processTransport();

  juce::AudioSampleBuffer monoToStereoBufferIn;
  juce::AudioSampleBuffer monoToStereoBufferOut;
  double m_Tempo = 0;
  bool wasPlaying = false;

  std::vector<std::shared_ptr<mpc::engine::midi::ShortMessage>> midiOutputBuffer = std::vector<std::shared_ptr<mpc::engine::midi::ShortMessage>>(100);

public:
  bool shouldShowDisclaimer = true;
  std::function<void()> showAudioSettingsDialog = [](){};
  mpc::Mpc mpc;
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcAudioProcessor)
};
