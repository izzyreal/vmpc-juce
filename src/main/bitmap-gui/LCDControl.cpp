#include "LCDControl.hpp"

#include <lcdgui/Layer.hpp>
#include <lcdgui/screens/OthersScreen.hpp>
#include "Constants.hpp"

#include "ContentComponent.hpp"
#include "AuxLCDWindow.hpp"

#include "lcdgui/BasicStructs.hpp"

#include <raw_keyboard_input/src/Keyboard.h>

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;

LCDControl::LCDControl(mpc::Mpc& _mpc)
    : mpc(_mpc), ls(_mpc.getLayeredScreen())
{
    lcd = juce::Image(juce::Image::RGB, 496, 120, true);
    auto othersScreen = mpc.screens->get<OthersScreen>("others");
    othersScreen->addObserver(this);
   resetAuxWindowF = [&]{ resetAuxWindow(); };
   getLcdImage = [&] () -> juce::Image& { return lcd; };
   resetKeyboardAuxParent = [&] {
        auto contentComponent = dynamic_cast<ContentComponent *>(getParentComponent());
        contentComponent->keyboard->setAuxParent(nullptr);
   };
}

void LCDControl::update(mpc::Observable *, mpc::Message message)
{
    const auto msg = std::get<std::string>(message);

    if (msg == "contrast")
    {
        ls->getFocusedLayer()->SetDirty(); // Could be done less invasively by just redrawing the current pixels of the LCD screens, but with updated colors
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
            else
            {
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

void LCDControl::resetAuxWindow()
{
    if (auxWindow != nullptr)
    {
        auxWindow->removeFromDesktop();
        delete auxWindow; auxWindow = nullptr;
    }
}

void LCDControl::checkLsDirty()
{
    if (ls->IsDirty())
    {
        auto dirtyArea = ls->getDirtyArea();
        dirtyRect = juce::Rectangle<int>(dirtyArea.L, dirtyArea.T, dirtyArea.W(), dirtyArea.H());
        const auto dirtyRectForAuxLcd = dirtyRect;
        ls->Draw();
        drawPixelsToImg();
        auto dirtyRect_x2 = juce::Rectangle<int>(dirtyArea.L * 2, dirtyArea.T * 2, dirtyArea.W() * 2, dirtyArea.H() * 2);
        repaint(dirtyRect_x2.expanded(1));

        if (auxWindow != nullptr)
        {
            auxWindow->repaintAuxLcdLocalBounds(dirtyRectForAuxLcd);
        }
    }
}

void LCDControl::timerCallback()
{
    checkLsDirty();
}

void LCDControl::paint(juce::Graphics &g)
{
    g.drawImageAt(lcd, 0, 0);
}

void LCDControl::mouseDoubleClick(const juce::MouseEvent &)
{
    if (auxWindow == nullptr)
    {
        auxWindow = new AuxLCDWindow(resetAuxWindowF, getLcdImage, resetKeyboardAuxParent, Constants::LCD_OFF);
        auto contentComponent = dynamic_cast<ContentComponent*>(getParentComponent());
        contentComponent->keyboard->setAuxParent(auxWindow);
    }
    else
    {
        auto contentComponent = dynamic_cast<ContentComponent*>(getParentComponent());
        contentComponent->keyboard->setAuxParent(nullptr);
        delete auxWindow;
        auxWindow = nullptr;
    }
}

LCDControl::~LCDControl()
{
    auto othersScreen = mpc.screens->get<OthersScreen>("others");
    othersScreen->deleteObserver(this);
    delete auxWindow;
}
