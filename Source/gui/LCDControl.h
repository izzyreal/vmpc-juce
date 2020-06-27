#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "VmpcComponent.h"

#include <vector>
#include <memory>

namespace mpc {
	namespace lcdgui {
		class LayeredScreen;
	}
}

class LCDControl
	: public VmpcComponent
	, public Timer {

private:
	std::weak_ptr<mpc::lcdgui::LayeredScreen> ls;
	Image lcd;
public:
	Rectangle<int> dirtyRect;

public:
	void checkLsDirty();
	void drawPixelsToImg();

public:
	void paint(Graphics& g) override;
	void timerCallback() override;

public:
	LCDControl(const String& componentName, std::weak_ptr<mpc::lcdgui::LayeredScreen> ls);

};
