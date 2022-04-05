#pragma once
#include "Keyboard.h"

class MacOsKeyboard : public Keyboard {
public:
  MacOsKeyboard();
  ~MacOsKeyboard();

private:
  void installMonitor();
  void removeMonitor();
  
  void* keyDownMonitor = nullptr;
  void* keyUpMonitor = nullptr;
  void* modifierChangedMonitor = nullptr;
};
