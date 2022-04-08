#pragma once

#include <set>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace juce { class ComponentPeer; }

class Keyboard : public juce::Component, public juce::Timer {

public:
  Keyboard();
  ~Keyboard() override;
  
  void timerCallback() override;

  juce::ComponentPeer* peer = nullptr;
  
  static bool processKeyEvent(int keyCode, bool isKeyDown);

  bool isKeyDown(int keyCode);
  void allKeysUp();
  
  std::function<void(int)> onKeyDownFn;
  std::function<void(int)> onKeyUpFn;

protected:
  static std::set<Keyboard*> thisses;

  static juce::ComponentPeer* getFocusedPeer();
  
  void addPressedKey(int keyCode);
  void removePresedKey(int keyCode);

private:
  std::set<int> pressedKeys;
  
};
