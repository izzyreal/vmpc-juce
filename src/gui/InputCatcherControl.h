#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>
#include <memory>

using namespace juce;

namespace mpc { class Mpc; }

class InputCatcherControl
	: public juce::Component
{

private:
	mpc::Mpc& mpc;
	std::vector<int> pressedKeys;

public:
	bool keyPressed(const KeyPress &key) override;
	bool keyStateChanged(bool isKeyDown) override;
	void modifierKeysChanged(const ModifierKeys& modifiers) override;
	void focusLost(FocusChangeType cause) override;
	
public:
	InputCatcherControl(mpc::Mpc& mpc, const String& componentName);

};
