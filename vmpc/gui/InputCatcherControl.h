#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <vector>
#include <memory>

namespace mpc {
	class Mpc;
	namespace controls {
		class KbMapping;
	}
}

class InputCatcherControl
	: public Component {

private:
	mpc::Mpc* mpc;
	mpc::controls::KbMapping* kbMapping;
	std::vector<int> pressedKeys;

public:
	bool keyPressed(const KeyPress &key) override;
	bool keyStateChanged(bool isKeyDown) override;
	void modifierKeysChanged(const ModifierKeys& modifiers) override;

public:
	InputCatcherControl(const String& componentName, mpc::Mpc* mpc);
	~InputCatcherControl();

};
