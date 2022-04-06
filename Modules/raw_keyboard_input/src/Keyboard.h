#pragma once

#include <set>
#include <functional>

namespace juce { class ComponentPeer; }

class Keyboard {
public:
	juce::ComponentPeer* peer = nullptr;
  bool isKeyDown(int keyCode);
  void allKeysUp();
  
  std::function<void(int)> onKeyDownFn;
  std::function<void(int)> onKeyUpFn;

protected:
  void addPressedKey(int keyCode);
  void removePresedKey(int keyCode);

private:
  std::set<int> pressedKeys;
  
};
