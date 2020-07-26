#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "VmpcComponent.h"

#include <vector>
#include <memory>

namespace mpc::lcdgui
{
	class LayeredScreen;
}

class LCDControl
	: public VmpcComponent
	, public Timer
{

private:
	std::weak_ptr<mpc::lcdgui::LayeredScreen> ls;
	Image lcd;
	Rectangle<int> dirtyRect;
	bool showEmpty = true;
	int showEmptyCount = 0;
	bool showBlack = true;
	int showBlackCount = 0;
	bool showHalfBlack = true;
	int showHalfBlackCount = 0;
	bool showMPC2000XL = true;
	int showMPC2000XLCount = 0;
	bool poweringUp = false;
	bool poweredUp = false;

public:
	void startPowerUpSequence();
	void checkLsDirty();
	void drawPixelsToImg();
	void paint(Graphics& g) override;
	void timerCallback() override;

public:
	LCDControl(const String& componentName, std::weak_ptr<mpc::lcdgui::LayeredScreen> ls);

};
