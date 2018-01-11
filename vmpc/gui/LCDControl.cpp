#include "LCDControl.h"

#include <lcdgui/LayeredScreen.hpp>
#include "Constants.h"
//#include <mpc/gui/ControlPanel.hpp>

#include <Logger.hpp>

//#include  "../resource.h"

LCDControl::LCDControl(const String& componentName, std::weak_ptr<mpc::lcdgui::LayeredScreen> ls)
	: VmpcComponent(componentName)
{
	this->ls = ls;
	lcd = Image(Image::ARGB, 496, 120, true);
}

void LCDControl::drawPixelsToImg() {
	auto pixels = ls.lock()->getPixels();
	Colour c;
	for (int x = 0; x < 248; x++) {
		for (int y = 0; y < 60; y++) {
			if (pixels->at(x).at(y) == true) {
				c = Constants::LCD_HALF_ON;
				lcd.setPixelAt(x * 2, y * 2, Constants::LCD_ON);
			}
			else {
				c = Constants::LCD_OFF;
				lcd.setPixelAt(x * 2, y * 2, c);
			}
			lcd.setPixelAt((x * 2) + 1, (y * 2), c);
			lcd.setPixelAt((x * 2) + 1, (y * 2) + 1, c);
			lcd.setPixelAt((x * 2), (y * 2) + 1, c);
		}
	}
}

void LCDControl::checkLsDirty() {
	if (ls.lock()->IsDirty()) {
		ls.lock()->Draw();
		drawPixelsToImg();
		repaint();
	}
}

void LCDControl::timerCallback() {
	checkLsDirty();
}

void LCDControl::paint(Graphics& g)
{
	g.drawImageAt(lcd, 0, 0);
}

LCDControl::~LCDControl() {
	MLOG("LCDControl dtor");
}
