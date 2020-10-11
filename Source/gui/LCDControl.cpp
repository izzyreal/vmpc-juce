#include "LCDControl.h"

#include <lcdgui/LayeredScreen.hpp>
#include <lcdgui/Layer.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/screens/OthersScreen.hpp>
#include "Constants.h"

#include <Logger.hpp>
#include <gui/BasicStructs.hpp>

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;

LCDControl::LCDControl(mpc::Mpc& mpc, const String& componentName, std::weak_ptr<mpc::lcdgui::LayeredScreen> ls)
	: VmpcComponent(componentName), mpc(mpc)
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

void LCDControl::startPowerUpSequence()
{
	poweringUp = true;
}

void LCDControl::skipPowerUpSequence()
{
	poweredUp = true;
	ls.lock()->getFocusedLayer().lock()->getParent()->SetDirty();
	drawPixelsToImg();
	repaint();
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
	if (!poweringUp && !poweredUp)
	{
		return;
	}

	static auto focus = getCurrentlyFocusedComponent();
	if (focus != getCurrentlyFocusedComponent())
	{
		if (getCurrentlyFocusedComponent() != nullptr)
			MLOG("focus: " + getCurrentlyFocusedComponent()->getName().toStdString());
		focus = getCurrentlyFocusedComponent();
	}

	if (!poweredUp)
	{
		if (showEmpty)
		{
			if (showEmptyCount == 0)
			{
				ls.lock()->openScreen("empty");
			}

			showEmptyCount++;
			
			if (showEmptyCount == 5)
			{
				showEmpty = false;
			}
		}
		else if (showBlack)
		{
			if (showBlackCount == 0)
			{
				ls.lock()->openScreen("black");
			}

			showBlackCount++;

			if (showBlackCount == 5)
			{
				showBlack = false;
			}
		}
		else if (showHalfBlack)
		{
			if (showHalfBlackCount == 0)
			{
				ls.lock()->openScreen("half-black");
			}

			showHalfBlackCount++;

			if (showHalfBlackCount == 1)
			{
				showHalfBlack = false;
			}
		}
		else if (showMPC2000XL)
		{
			if (showMPC2000XLCount == 0)
			{
				ls.lock()->openScreen("mpc2000xl");
			}

			showMPC2000XLCount++;

			if (showMPC2000XLCount == 12)
			{
				showMPC2000XL = false;
			}
		}
		else
		{
			poweringUp = false;
			poweredUp = true;
			ls.lock()->openScreen("sequencer");
		}
	}

	checkLsDirty();
}

void LCDControl::paint(Graphics& g)
{
	g.drawImageAt(lcd, 0, 0);
}
