#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/WithSharedTimerCallback.hpp"
#include "gui/LcdInteraction.hpp"

#include <Observer.hpp>
#include <lcdgui/LcdGeometry.hpp>

#include <melatonin_blur/melatonin/shadows.h>

namespace vmpc_juce::gui
{
    class AuxLcdWindow;
}

namespace mpc
{
    class Mpc;
}

namespace vmpc_juce::gui::vector
{
    class View;

    class Lcd final : public juce::Component,
                      public WithSharedTimerCallback,
                      public mpc::Observer
    {
    public:
        explicit Lcd(mpc::Mpc &mpcToUse);

        ~Lcd() override;

        void update(mpc::Observable *, mpc::Message message) override;

        void paint(juce::Graphics &g) override;

        void checkLsDirty();

        void sharedTimerCallback() override;

        void drawPixelsToImg();

        void mouseDown(const juce::MouseEvent &e) override;

        void mouseUp(const juce::MouseEvent &e) override;

        void mouseDrag(const juce::MouseEvent &e) override;

        void mouseWheelMove(const juce::MouseEvent &e,
                            const juce::MouseWheelDetails &wheel) override;

        bool toggleAuxWindow();
        void closeAuxWindow();
        bool isAuxWindowOpen() const;

        float magicMultiplier = 0.55f;

    private:
        mpc::Mpc &mpc;
        AuxLcdWindow *auxWindow = nullptr;
        juce::Rectangle<int> dirtyRect;
        juce::Image img = juce::Image(juce::Image::PixelFormat::RGB,
                                      mpc::lcdgui::LCD_WIDTH * 2,
                                      mpc::lcdgui::LCD_HEIGHT * 2, false);

        std::function<juce::Image &()> getLcdImage;

        melatonin::DropShadow shadow;

        juce::AffineTransform getMyTransform() const;
        juce::Rectangle<float> getRenderedLcdBounds() const;

        View *getView() const;
    };
} // namespace vmpc_juce::gui::vector
