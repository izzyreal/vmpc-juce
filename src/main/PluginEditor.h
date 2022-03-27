#pragma once

#include "juce_gui_extra/juce_gui_extra.h"
#include "PluginProcessor.h"

#include "gui/KeyEventListener.h"

using namespace juce;

namespace mpc { class Mpc; }

class VmpcAudioProcessorEditor
: public AudioProcessorEditor
{
  
public:
  explicit VmpcAudioProcessorEditor(VmpcAudioProcessor&);
  ~VmpcAudioProcessorEditor() override;
  
  void resized() override;
  
private:
  void initialise();
  
private:
  VmpcAudioProcessor& vmpcAudioProcessor;
  mpc::Mpc& mpc;
  
  Viewport viewport;
  
  TooltipWindow tooltipWindow { this, 300 };
  Component::SafePointer<SplashScreen> mpcSplashScreen;

  Image bgImg;
  
  VmpcAudioProcessor& getProcessor() const
  {
    return static_cast<VmpcAudioProcessor&> (processor);
  }
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcAudioProcessorEditor)
};
