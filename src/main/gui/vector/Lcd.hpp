#pragma once

#include <Observer.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur/melatonin/shadows.h>

namespace mpc
{
    class Mpc;
}

namespace vmpc_juce::gui
{
    class AuxLCDWindow;
}

namespace vmpc_juce::gui::vector
{
    class View;

    class Lcd final : public juce::Component, juce::Timer, public mpc::Observer
    {
    public:
        explicit Lcd(mpc::Mpc &);
        ~Lcd() override;

        void update(mpc::Observable *, mpc::Message) override;
        void paint(juce::Graphics &g) override;
        void timerCallback() override;
        void mouseDoubleClick(const juce::MouseEvent &) override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseDrag(const juce::MouseEvent &e) override;

        float magicMultiplier = 0.55f;

    private:
        void checkLsDirty();
        void drawPixelsToImg();
        juce::AffineTransform getMyTransform() const;
        void resetAuxWindow();
        View *getView() const;

        mpc::Mpc &mpc;
        AuxLCDWindow *auxWindow = nullptr;
        juce::Rectangle<int> dirtyRect;
        juce::Image img =
            juce::Image(juce::Image::PixelFormat::RGB, 248 * 2, 60 * 2, false);
        std::function<void()> resetAuxWindowF;
        std::function<void()> resetKeyboardAuxParent;
        std::function<juce::Image &()> getLcdImage;
        melatonin::DropShadow shadow;
    };
} // namespace vmpc_juce::gui::vector
