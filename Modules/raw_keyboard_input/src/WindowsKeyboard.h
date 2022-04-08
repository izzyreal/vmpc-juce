#pragma once
#include "Keyboard.h"

#include <Windows.h>

class WindowsKeyboard : public Keyboard {
public:
  WindowsKeyboard();
  ~WindowsKeyboard();
  
private:
	static LRESULT CALLBACK keyHandler(int keyCode, WPARAM w, LPARAM l);

};
