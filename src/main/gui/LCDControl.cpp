#include "LCDControl.h"

#include <lcdgui/LayeredScreen.hpp>
#include <lcdgui/Layer.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/screens/OthersScreen.hpp>
#include "Constants.h"

#include <gui/BasicStructs.hpp>

using namespace juce;
using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;

LCDControl::LCDControl(mpc::Mpc& mpc, std::weak_ptr<mpc::lcdgui::LayeredScreen> ls)
	: mpc(mpc)
{
	this->ls = ls;
	lcd = Image(Image::RGB, 496, 120, true);
	auto othersScreen = std::dynamic_pointer_cast<OthersScreen>(mpc.screens->getScreenComponent("others"));
	othersScreen->addObserver(this);
}

void LCDControl::update(moduru::observer::Observable* o, nonstd::any msg)
{
	auto message = nonstd::any_cast<std::string>(msg);

	if (message.compare("contrast") == 0)
	{
		ls.lock()->getFocusedLayer().lock()->SetDirty(); // Could be done less invasively by just redrawing the current pixels of the LCD screens, but with updated colors
		repaint();
	}
}

void LCDControl::drawPixelsToImg()
{
	auto pixels = ls.lock()->getPixels();
	
	auto othersScreen = std::dynamic_pointer_cast<OthersScreen>(mpc.screens->getScreenComponent("others"));
	auto contrast = othersScreen->getContrast();
	
	Colour c;
	
	auto halfOn = Constants::LCD_HALF_ON.darker(contrast / 50.0);
	auto on = Constants::LCD_ON.darker(contrast / 50.0);
	auto off = Constants::LCD_OFF.brighter(contrast / 70.0);

	const auto rectX = dirtyRect.getX();
	const auto rectY = dirtyRect.getY();
	const auto rectRight = dirtyRect.getRight();
	const auto rectBottom = dirtyRect.getBottom();

	for (int x = rectX; x < rectRight; x++)
	{
		for (int y = rectY; y < rectBottom; y++)
		{
			const auto x_x2 = x * 2;
			const auto y_x2 = y * 2;
			
			if ((*pixels)[x][y])
			{
				c = halfOn;
				lcd.setPixelAt(x_x2, y_x2, on);
			}
			else {
				c = off;
				lcd.setPixelAt(x_x2, y_x2, c);
			}
					
			lcd.setPixelAt(x_x2 + 1, y_x2, c);
			lcd.setPixelAt(x_x2 + 1, y_x2 + 1, c);
			lcd.setPixelAt(x_x2, y_x2 + 1, c);
		}
	}
	dirtyRect = Rectangle<int>();
}

void LCDControl::checkLsDirty()
{
	if (ls.lock()->IsDirty())
	{
		auto dirtyArea = ls.lock()->getDirtyArea();
		dirtyRect = Rectangle<int>(dirtyArea.L, dirtyArea.T, dirtyArea.W(), dirtyArea.H());
		ls.lock()->Draw();
		drawPixelsToImg();
		repaint();
	}
}

void LCDControl::timerCallback()
{
	checkLsDirty();
}

void LCDControl::paint(Graphics& g)
{
	g.drawImageAt(lcd, 0, 0);
}
