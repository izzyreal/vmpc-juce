#include "Keyboard.h"

#include <set>

class MacOsKeyboard : public Keyboard {
public:
  MacOsKeyboard();
  ~MacOsKeyboard();
  
  bool isKeyDown(int keyCode);
    
private:
  void installMonitor();
  void removeMonitor();
  
  void* keyDownMonitor = nullptr;
  void* keyUpMonitor = nullptr;
  void* modifierChangedMonitor = nullptr;
};
