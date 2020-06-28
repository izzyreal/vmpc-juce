#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <vector>
#include <memory>

class InputCatcherControl
	: public Component {

private:
	std::vector<int> pressedKeys;

public:
	bool keyPressed(const KeyPress &key) override;
	bool keyStateChanged(bool isKeyDown) override;
	void modifierKeysChanged(const ModifierKeys& modifiers) override;
	void focusLost(FocusChangeType cause) override;

public:
	InputCatcherControl(const String& componentName);

};
