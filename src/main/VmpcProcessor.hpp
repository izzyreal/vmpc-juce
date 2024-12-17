/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <Mpc.hpp>

namespace mpc::engine::midi { class ShortMessage; }

namespace vmpc_juce {

class VmpcProcessor  : public juce::AudioProcessor
{
public:
  //==============================================================================
  VmpcProcessor();
  ~VmpcProcessor() override;

  //==============================================================================
  void prepareToPlay (double sampleRate, int samplesPerBlock) override;

  void releaseResources() override {}
  
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
  void processMidiOut(juce::MidiBuffer& midiMessages, bool discard);
  void processTransport();

  juce::AudioSampleBuffer monoToStereoBufferIn;
  juce::AudioSampleBuffer monoToStereoBufferOut;
  double m_Tempo = 0;
  bool wasPlaying = false;
  int framesProcessed = 0;

  std::vector<std::shared_ptr<mpc::engine::midi::ShortMessage>> midiOutputBuffer = std::vector<std::shared_ptr<mpc::engine::midi::ShortMessage>>(100);

public:
  bool shouldShowDisclaimer = true;
  std::function<void()> showAudioSettingsDialog = [](){};
  mpc::Mpc mpc;
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcProcessor)
};

} // namespace vmpc_juce
