#pragma once
#include "Observer.hpp"
#include "juce_graphics/juce_graphics.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include "Constants.hpp"
#include "View.hpp"
#include "melatonin_blur/melatonin/shadows.h"

#include "Mpc.hpp"
#include "lcdgui/Screens.hpp"
#include "lcdgui/Layer.hpp"
#include "lcdgui/screens/OthersScreen.hpp"

#include "../bitmap-gui/AuxLCDWindow.hpp"

#include <raw_keyboard_input/raw_keyboard_input.h>

using namespace mpc::lcdgui::screens;

class AuxLCD;

class Lcd : public juce::Component, juce::Timer, public mpc::Observer {
    public:
        Lcd(mpc::Mpc &mpcToUse) : mpc(mpcToUse)
    {
        resetAuxWindowF = [&] { resetAuxWindow(); };
        resetKeyboardAuxParent = [&] { getView()->keyboard->setAuxParent(nullptr); };
        getLcdImage = [&]() -> juce::Image& { return img; };
        drawPixelsToImg();
        startTimer(25);
        auto othersScreen = mpc.screens->get<OthersScreen>("others");
        othersScreen->addObserver(this);
    }

        ~Lcd() override
        {
            auto othersScreen = mpc.screens->get<OthersScreen>("others");
            othersScreen->deleteObserver(this);
            delete auxWindow;
        }

        void update(mpc::Observable *, mpc::Message message) override
        {
            const auto msg = std::get<std::string>(message);

            if (msg == "contrast")
            {
                mpc.getLayeredScreen()->getFocusedLayer()->SetDirty(); // Could be done less invasively by just redrawing the current pixels of the LCD screens, but with updated colors
                repaint();
            }
        }

        void paint(juce::Graphics &g) override
        {
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

            const auto layeredScreen = mpc.getLayeredScreen();

            const auto t = getTransform();

            g.drawImageTransformed(img, t);
            //////////// SHADOW ///////////////
            auto color = Constants::lcdOffBacklit.brighter().withAlpha(0.4f);
            int radius = (int) std::round<float>((float)getWidth() / 248.f);
            juce::Point<int> offset = { 0, 0 };
            int spread = 0.f;
            melatonin::DropShadow shadow = { color, radius, offset, spread };

            juce::Path p;

            const auto &rawPixels = *layeredScreen->getPixels();

            for (uint8_t y = 0; y < 60; y++)
            {
                for (uint8_t x = 0; x < 248; x++)
                {
                    const bool on = rawPixels[x][y];
                    if (!on) p.addRectangle(x*2, y*2, 2, 2);
                }
            }

            p.applyTransform(t);
            shadow.render(g, p);
        }

        void checkLsDirty()
        {
            const auto layeredScreen = mpc.getLayeredScreen();

            if (!layeredScreen->IsDirty())
            {
                return;
            }

            const auto dirtyArea = layeredScreen->getDirtyArea();
            dirtyRect = juce::Rectangle<int>(dirtyArea.L, dirtyArea.T, dirtyArea.W(), dirtyArea.H());
            const auto dirtyRectForAuxLcd = dirtyRect;
            const auto dirtyRectT = dirtyRect.toFloat().transformedBy(juce::AffineTransform().scaled(2.f)).transformedBy(getTransform());

            layeredScreen->Draw();
            drawPixelsToImg();

            repaint(dirtyRectT.toNearestInt().expanded(1));

            if (auxWindow != nullptr)
            {
                auxWindow->repaintAuxLcdLocalBounds(dirtyRectForAuxLcd);
            }
        }

        void timerCallback() override
        {
            checkLsDirty();
        }

        void drawPixelsToImg()
        {
            const auto layeredScreen = mpc.getLayeredScreen();

            const auto pixels = layeredScreen->getPixels();

            auto othersScreen = mpc.screens->get<OthersScreen>("others");
            auto contrast = othersScreen->getContrast();

            juce::Colour c;

            auto halfOn = Constants::lcdOnLight.darker(static_cast<float>(contrast * 0.02));
            auto on = Constants::lcdOn.darker(static_cast<float>(contrast * 0.02));
            auto off = Constants::lcdOff.brighter(static_cast<float>(contrast * 0.01428));

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
                        img.setPixelAt(x_x2, y_x2, on);
                    }
                    else
                    {
                        c = off;
                        img.setPixelAt(x_x2, y_x2, c);
                    }

                    img.setPixelAt(x_x2 + 1, y_x2, c);
                    img.setPixelAt(x_x2 + 1, y_x2 + 1, c);
                    img.setPixelAt(x_x2, y_x2 + 1, c);
                }
            }

            dirtyRect = juce::Rectangle<int>();
        }

        void mouseDoubleClick (const juce::MouseEvent&) override
        {
            auto view = getView();

            if (auxWindow == nullptr)
            {
                auxWindow = new AuxLCDWindow(resetAuxWindowF, getLcdImage, resetKeyboardAuxParent, Constants::lcdOff);
                view->keyboard->setAuxParent(auxWindow);
            }
            else
            {
                view->keyboard->setAuxParent(nullptr);
                delete auxWindow;
                auxWindow = nullptr;
            }
        }

        void mouseDown(const juce::MouseEvent& e) override {
            getParentComponent()->mouseDown(e);
        }

        void mouseDrag(const juce::MouseEvent& e) override {
            getParentComponent()->mouseDrag(e);
        }

        float magicMultiplier = 0.55f;

    private:
        mpc::Mpc &mpc;
        AuxLCDWindow* auxWindow = nullptr;
        juce::Rectangle<int> dirtyRect;
        juce::Image img = juce::Image(juce::Image::PixelFormat::RGB, 248*2, 60*2, false);

        std::function<void()> resetAuxWindowF;
        std::function<void()> resetKeyboardAuxParent;
        std::function<juce::Image&()> getLcdImage;

        juce::AffineTransform getTransform()
        {
            const auto asp_ratio = 60.f/248.f;
            const auto w = float(getWidth()) * magicMultiplier;
            const auto h = w * asp_ratio;
            const auto img_scale = w / (248 * 2);
            const auto unused_h_px = getWidth() - w;
            const auto unused_v_px = getHeight() - h;
            const auto x_offset = unused_h_px * 0.5f;
            const auto y_offset = unused_v_px * 0.5f;

            juce::AffineTransform t;
            t = t.scaled(img_scale);
            t = t.translated(x_offset, y_offset);
            return t; 
        }

        void resetAuxWindow()
        {
            if (auxWindow != nullptr)
            {
                auxWindow->removeFromDesktop();
                delete auxWindow; auxWindow = nullptr;
            }
        }

        View* getView()
        {
            juce::Component *ancestor = getParentComponent();

            while(dynamic_cast<View*>(ancestor) == nullptr)
            {
                ancestor = ancestor->getParentComponent();
            }

            return dynamic_cast<View*>(ancestor);
        }
};

