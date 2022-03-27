#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace mpc { class Mpc; }

namespace mpc::controls {
class KeyEventHandler;
}

class KeyEventListener
: public juce::Component
{
public:
  KeyEventListener(mpc::Mpc&);
  bool keyPressed(const juce::KeyPress &key) override;
  bool keyEvent(const juce::KeyEvent &keyEvent) override;
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;
  
private:
  mpc::Mpc& mpc;
  std::weak_ptr<mpc::controls::KeyEventHandler> keyEventHandler;
  std::vector<std::shared_ptr<juce::MouseInputSource>> sources;
  float prevDistance = -1.f;
  float prevPinchCx = -1.f;
  float prevPinchCy = -1.f;
  float prevSingleX = -1.f;
  float prevSingleY = -1.f;
};
