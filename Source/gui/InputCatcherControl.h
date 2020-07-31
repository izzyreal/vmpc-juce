#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <vector>
#include <memory>

namespace mpc { class Mpc; }

class InputCatcherControl
	: public Component {

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
