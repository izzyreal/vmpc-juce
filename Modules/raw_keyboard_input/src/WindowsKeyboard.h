#pragma once
#include "Keyboard.h"

#include <Windows.h>

#include <set>

#include <juce_gui_basics/juce_gui_basics.h>

class WindowsKeyboard : public Keyboard {
public:
  WindowsKeyboard();
  ~WindowsKeyboard();

private:
	static std::set<WindowsKeyboard*> thisses;
	static LRESULT CALLBACK keyHandler2(int keyCode, WPARAM w, LPARAM l);


};
