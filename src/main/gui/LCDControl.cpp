#include "LCDControl.h"

#include "AuxLCD.h"

#include <lcdgui/Layer.hpp>
#include <lcdgui/screens/OthersScreen.hpp>
#include "Constants.h"

#include "ContentComponent.h"

#include <gui/BasicStructs.hpp>

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;

LCDControl::LCDControl(mpc::Mpc &_mpc, Keyboard* keyboard)
        : mpc(_mpc), ls(_mpc.getLayeredScreen())
{
    lcd = juce::Image(juce::Image::RGB, 496, 120, true);
    auto othersScreen = mpc.screens->get<OthersScreen>("others");
    othersScreen->addObserver(this);
}

void LCDControl::update(moduru::observer::Observable *, nonstd::any msg)
{
    auto message = nonstd::any_cast<std::string>(msg);

    if (message == "contrast")
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

void LCDControl::checkLsDirty()
{
    if (ls->IsDirty())
    {
        auto dirtyArea = ls->getDirtyArea();
        dirtyRect = juce::Rectangle<int>(dirtyArea.L, dirtyArea.T, dirtyArea.W(), dirtyArea.H());
        ls->Draw();
        drawPixelsToImg();
        auto dirtyRect_x2 = juce::Rectangle<int>(dirtyArea.L * 2, dirtyArea.T * 2, dirtyArea.W() * 2, dirtyArea.H() * 2);
        repaint(dirtyRect_x2);

        if (auxLcd != nullptr)
        {
            auto auxBounds = auxLcd->getLocalBounds();

            auto scale = auxBounds.getWidth() / (248.f);

            auto auxRepaintBounds = juce::Rectangle<int>(dirtyArea.L * scale, dirtyArea.T * scale, dirtyArea.W() * scale, dirtyArea.H() * scale);

            auxLcd->repaint(auxRepaintBounds.expanded(3));
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
    if (auxWindow != nullptr)
    {
        auto contentComponent = dynamic_cast<ContentComponent *>(getParentComponent());
        contentComponent->keyboard->setAuxParent(nullptr);
        delete auxWindow;
        auxWindow = nullptr;
        delete auxLcd;
        auxLcd = nullptr;
    }
    else
    {
        auxWindow = new juce::ResizableWindow("foo", true);
        auxWindow->setOpaque(false);
        auxWindow->setVisible(true);
        const int margin = 0;
        auxWindow->setResizable(true, true);

        const int minWidth = 248 + (margin * 2);
        const int minHeight = 60 + (margin * 2);
        const int maxWidth = (248 * 8) + (margin * 2);
        const int maxHeight = (60 * 8) + (margin * 2);

        auxWindow->setResizeLimits(minWidth, minHeight, maxWidth, maxHeight);
        auxWindow->getConstrainer()->setFixedAspectRatio((float) minWidth / minHeight);
        auxWindow->setBounds(0, 0, (248 * 3) + (margin * 2), (60 * 3) + (margin * 2));
        auxWindow->setAlwaysOnTop(true);
        auxWindow->setWantsKeyboardFocus(true);

        auto contentComponent = dynamic_cast<ContentComponent *>(getParentComponent());
        contentComponent->keyboard->setAuxParent(auxWindow);

        auxLcd = new AuxLCD(this, contentComponent->keyboard);

        auxWindow->setContentOwned(auxLcd, false);
        auxWindow->setBackgroundColour(Constants::LCD_OFF);
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
