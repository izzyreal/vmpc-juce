#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "DataWheelControl.h"
#include "LCDControl.h"
#include "ButtonControl.hpp"
#include "PadControl.hpp"
#include "SliderControl.hpp"
#include "LedControl.hpp"
#include "KnobControl.hpp"

#include <vector>

namespace mpc { class Mpc; }

namespace mpc::controls {
class KeyEventHandler;
}

class KeyEventListener
: public juce::Component
{
public:
  KeyEventListener(mpc::Mpc&);
  ~KeyEventListener() override;
  
  bool keyPressed(const juce::KeyPress &key) override;
  bool keyEvent(const juce::KeyEvent &keyEvent) override;
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;
  void resized() override;
  
private:
  mpc::Mpc& mpc;
  std::weak_ptr<mpc::controls::KeyEventHandler> keyEventHandler;
  std::vector<std::shared_ptr<juce::MouseInputSource>> sources;
  float prevDistance = -1.f;
  float prevPinchCx = -1.f;
  float prevPinchCy = -1.f;
  float prevSingleX = -1.f;
  float prevSingleY = -1.f;
  
  juce::Image dataWheelImg;
  juce::Image padHitImg;
  juce::Image sliderImg;
  juce::Image recKnobImg;
  juce::Image volKnobImg;
  juce::Image ledRedImg;
  juce::Image ledGreenImg;
  juce::Image keyboardImg;
  juce::Image resetWindowSizeImg;

  juce::Label versionLabel;
  
  juce::ImageButton keyboardButton;
  juce::ImageButton resetWindowSizeButton;

  DataWheelControl* dataWheel;
  LCDControl* lcd;
  std::vector<ButtonControl*> buttons;
  std::vector<PadControl*> pads;

  KnobControl* recKnob;
  KnobControl* volKnob;
  SliderControl* slider;
  LedControl* leds;

};
