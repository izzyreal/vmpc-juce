#include "LCDControl.h"

#include <lcdgui/LayeredScreen.hpp>
#include <lcdgui/Layer.hpp>
#include <lcdgui/screens/OthersScreen.hpp>
#include "Constants.h"

#include <gui/BasicStructs.hpp>

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;

LCDControl::LCDControl(mpc::Mpc& _mpc)
	: mpc (_mpc), ls (_mpc.getLayeredScreen().lock())
{
	lcd = juce::Image(juce::Image::RGB, 496, 120, true);
	auto othersScreen = mpc.screens->get<OthersScreen>("others");
	othersScreen->addObserver(this);
}

void LCDControl::update(moduru::observer::Observable*, nonstd::any msg)
{
	auto message = nonstd::any_cast<std::string>(msg);

	if (message == "contrast")
	{
		ls->getFocusedLayer().lock()->SetDirty(); // Could be done less invasively by just redrawing the current pixels of the LCD screens, but with updated colors
		repaint();
	}
}

void LCDControl::drawPixelsToImg()
{
	auto pixels = ls->getPixels();
	
	auto othersScreen = mpc.screens->get<OthersScreen>("others");
	auto contrast = othersScreen->getContrast();

  juce::Colour c;
	
	auto halfOn = Constants::LCD_HALF_ON.darker(static_cast<float>(contrast * 0.02));
  auto on = Constants::LCD_ON.darker(static_cast<float>(contrast * 0.02));
	auto off = Constants::LCD_OFF.brighter(static_cast<float>(contrast * 0.01428));

    if (isAux) dirtyRect = juce::Rectangle<int>(248, 60);

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
	dirtyRect = juce::Rectangle<int>();
}

bool LCDControl::auxNeedsToUpdate = false;

void LCDControl::checkLsDirty()
{
    if (isAux && auxNeedsToUpdate)
    {
        drawPixelsToImg();
        repaint();
        auxNeedsToUpdate = false;
    }
	else if (!isAux && ls->IsDirty())
	{
		auto dirtyArea = ls->getDirtyArea();
		dirtyRect = juce::Rectangle<int>(dirtyArea.L, dirtyArea.T, dirtyArea.W(), dirtyArea.H());
		ls->Draw();
		drawPixelsToImg();
		repaint();
        auxNeedsToUpdate = true;
	}
}

void LCDControl::timerCallback()
{
	checkLsDirty();
}

void LCDControl::paint(juce::Graphics& g)
{
    if (isAux)
    {
        g.drawImage(lcd, getLocalBounds().toFloat());
    }
	else
    {
        g.drawImageAt(lcd, 0, 0);
    }
}

void LCDControl::mouseDoubleClick (const juce::MouseEvent&)
{
    if (isAux)
    {
        return;
    }

    if (auxWindow != nullptr)
    {
        delete auxWindow;
        auxWindow = nullptr;
    }
    else
    {
        auxWindow = new juce::ResizableWindow("foo", true);
        auxWindow->setOpaque(false);
        auxWindow->setVisible (true);
        const int margin = 20;
        auxWindow->setResizable(true, true);
        auxWindow->setResizeLimits(248 + margin, 60 + margin, (248*8) + margin, (60 * 8) + margin);
        auxWindow->getConstrainer()->setFixedAspectRatio((496.f + margin) / (120.f + margin));
        auxWindow->setBounds(0, 0, 496 + margin, 120 + margin);

        class AuxLCD : public LCDControl {
        public: AuxLCD(mpc::Mpc& m) : LCDControl(m) {}
            void resized() override {
                setBounds(margin / 2, margin / 2, getParentWidth() - margin, getParentHeight() - margin);
            }
        };

        auto auxLcd = new AuxLCD(mpc);
        auxLcd->isAux = true;
        auxWindow->setContentOwned(auxLcd, false);
        auxLcd->drawPixelsToImg();
        auxLcd->startTimer(25);
    }
}

LCDControl::~LCDControl()
{
  auto othersScreen = mpc.screens->get<OthersScreen>("others");
  othersScreen->deleteObserver(this);

  if (auxWindow != nullptr)
  {
      delete auxWindow;
  }
}
