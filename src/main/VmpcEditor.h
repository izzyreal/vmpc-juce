#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "VmpcProcessor.h"

#include "gui/ContentComponent.h"

namespace mpc { class Mpc; }

class VmpcEditor
: public juce::AudioProcessorEditor
{
  
public:
  explicit VmpcEditor(VmpcProcessor&);
  ~VmpcEditor() override;
  
  void resized() override;
  
private:
  void showDisclaimer();
  
private:
  VmpcProcessor& vmpcProcessor;
  mpc::Mpc& mpc;

  juce::Viewport viewport;
  
  juce::TooltipWindow tooltipWindow { this, 300 };
  Component::SafePointer<juce::Component> vmpcSplashScreen;

  juce::Image bgImg;
  
  VmpcProcessor& getProcessor() const
  {
    return static_cast<VmpcProcessor&> (processor);
  }
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcEditor)
};
