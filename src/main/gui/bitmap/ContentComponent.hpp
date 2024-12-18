#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "Background.hpp"
#include "DataWheelControl.hpp"
#include "LCDControl.hpp"
#include "ButtonControl.hpp"
#include "PadControl.hpp"
#include "SliderControl.hpp"
#include "LedControl.hpp"
#include "KnobControl.hpp"
#include "TopRightMenu.hpp"

#include <vector>

class Keyboard;

namespace mpc { class Mpc; }
namespace mpc::controls { class KeyEventHandler; }

namespace vmpc_juce::gui::bitmap {

class ContentComponent : public juce::Component, juce::FocusChangeListener
{
public:
  ContentComponent(mpc::Mpc&, std::function<void()>& showAudioSettingsDialog);
  ~ContentComponent() override;

  Keyboard* keyboard = nullptr;

  bool keyPressed(const juce::KeyPress &key) override;
  void resized() override;
  void globalFocusChanged(juce::Component*) override;

private:
  mpc::Mpc& mpc;
  std::weak_ptr<mpc::controls::KeyEventHandler> keyEventHandler;
  std::vector<std::shared_ptr<juce::MouseInputSource>> sources;

  TopRightMenu* topRightMenu;
    
  juce::Image dataWheelImg;
  juce::Image padHitImg;
  juce::Image sliderImg;
  juce::Image recKnobImg;
  juce::Image volKnobImg;
  juce::Image ledRedImg;
  juce::Image ledGreenImg;
    
  juce::Label versionLabel;
    
  Background* background;
  DataWheelControl* dataWheel;
  LCDControl* lcd;

  std::vector<ButtonControl*> buttons;
  std::vector<PadControl*> pads;

  KnobControl* recKnob;
  KnobControl* volKnob;
  SliderControl* slider;
  LedControl* leds;

};

}
