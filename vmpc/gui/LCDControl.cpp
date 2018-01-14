#include "LCDControl.h"

#include <lcdgui/LayeredScreen.hpp>
#include "Constants.h"
//#include <mpc/gui/ControlPanel.hpp>

#include <Logger.hpp>
#include <gui/BasicStructs.hpp>
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
	for (int x = dirtyRect.getX(); x < dirtyRect.getRight(); x++) {
		for (int y = dirtyRect.getY(); y < dirtyRect.getBottom(); y++) {
			if ((*pixels)[x][y] == true) {
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
	dirtyRect = Rectangle<int>();
}

void LCDControl::checkLsDirty() {
	if (ls.lock()->IsDirty()) {
		ls.lock()->Draw();
		auto da = ls.lock()->getDirtyArea();
		da->L -= 1;
		da->B += 1;
		if (da->L < 0) da->L = 0;
		if (da->R > 248) da->R = 248;
		if (da->T < 0) da->T = 0;
		if (da->B > 60) da->B = 60;
		dirtyRect = Rectangle<int>(da->L, da->T, da->W(), da->H());
		//MLOG("dirty coords: " + std::to_string(da->L) + ", " + std::to_string(da->T) + ", " + std::to_string(da->R) + ", " + std::to_string(da->B));
		ls.lock()->getDirtyArea()->Clear();
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
