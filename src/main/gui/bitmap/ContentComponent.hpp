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
