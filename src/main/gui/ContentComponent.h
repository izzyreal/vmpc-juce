#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "Background.h"
#include "DataWheelControl.h"
#include "LCDControl.h"
#include "ButtonControl.hpp"
#include "PadControl.hpp"
#include "SliderControl.hpp"
#include "LedControl.hpp"
#include "KnobControl.hpp"

#include <vector>

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include "IosDocumentBrowser.h"
#include "Paths.hpp"

namespace mpc { class Mpc; }

class VmpcURLProcessor : public URLProcessor {
private:
    std::string destinationDir() {
        return mpc->paths->defaultLocalVolumePath();
    }
public:
  mpc::Mpc* mpc;
  bool destinationExists(const char* filename, const char* relativePath) override;
  std::shared_ptr<std::ostream> openOutputStream(const char* filename, const char* relativePath) override;
  void initFiles() override;
};

#define ENABLE_IMPORT 1

#endif
#endif

class KeyboardButton : public juce::ImageButton {
public:
    void mouseEnter(const juce::MouseEvent&) override {
        for (auto& c1 : getParentComponent()->getChildren())
            if (auto c2 = dynamic_cast<VmpcTooltipComponent*>(c1)) c2->showKeyboardMapping();
    }
    void mouseExit(const juce::MouseEvent&) override {
        for (auto& c1 : getParentComponent()->getChildren())
            if (auto c2 = dynamic_cast<VmpcTooltipComponent*>(c1)) c2->hideKeyboardMapping();
    }
};

class Keyboard;

namespace mpc { class Mpc; }

namespace mpc::controls {
class KeyEventHandler;
}

class ContentComponent
: public juce::Component, juce::FocusChangeListener
{
public:
  ContentComponent(mpc::Mpc&, std::function<void()>& showAudioSettingsDialog);
  ~ContentComponent() override;

  Keyboard* keyboard = nullptr;

  bool keyPressed(const juce::KeyPress &key) override;
  void resized() override;
  void globalFocusChanged(juce::Component*) override;

private:
#if ENABLE_IMPORT
  VmpcURLProcessor urlProcessor;
#endif
  mpc::Mpc& mpc;
  std::weak_ptr<mpc::controls::KeyEventHandler> keyEventHandler;
  std::vector<std::shared_ptr<juce::MouseInputSource>> sources;

  juce::Image dataWheelImg;
  juce::Image padHitImg;
  juce::Image sliderImg;
  juce::Image recKnobImg;
  juce::Image volKnobImg;
  juce::Image ledRedImg;
  juce::Image ledGreenImg;
  juce::Image helpImg;
  juce::Image gearImg;
  juce::Image keyboardImg;
  juce::Image resetWindowSizeImg;
  juce::Image importImg;

  juce::Label versionLabel;

  juce::ImageButton helpButton;
  juce::ImageButton gearButton;
  KeyboardButton keyboardButton;
  juce::ImageButton resetWindowSizeButton;
  juce::ImageButton importButton;

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
