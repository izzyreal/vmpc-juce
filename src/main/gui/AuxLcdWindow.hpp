#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/AuxLcd.hpp"
#include "gui/LcdInteraction.hpp"
#include "gui/VmpcAuxLcdLookAndFeel.hpp"

namespace mpc
{
    class Mpc;
}

namespace vmpc_juce::gui
{
    class AuxLcdWindowMaximizeButton final : public juce::Button
    {
    public:
        AuxLcdWindowMaximizeButton() : Button("MaximizeButton") {}
        void paint(juce::Graphics &g) override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseEnter(const juce::MouseEvent &e) override;

    protected:
        void paintButton(juce::Graphics &g,
                         bool /*shouldDrawButtonAsHighlighted*/,
                         bool /*shouldDrawButtonAsDown*/) override;
    };

    class AuxLcdWindow final : public juce::TopLevelWindow, public juce::Timer
    {
    public:
        explicit AuxLcdWindow(
            mpc::Mpc &mpc, const std::function<void()> &resetAuxWindowToUse,
            const std::function<juce::Image &()> &getLcdImage,
            const std::function<void()> &resetKeyboardAuxParentToUse,
            juce::Colour backgroundColourToUse);

        void timerCallback() override;

        bool keyPressed(const juce::KeyPress &) override;

        void paint(juce::Graphics &) override;

        void resized() override;

        ~AuxLcdWindow() override;

        void mouseMove(const juce::MouseEvent &) override;

        void mouseEnter(const juce::MouseEvent &) override;

        void mouseDown(const juce::MouseEvent &) override;

        void mouseUp(const juce::MouseEvent &) override;

        void mouseDrag(const juce::MouseEvent &) override;

        void mouseWheelMove(const juce::MouseEvent &,
                            const juce::MouseWheelDetails &) override;

        void mouseDoubleClick(const juce::MouseEvent &) override;

        void showButtons();

        void repaintAuxLcdLocalBounds(juce::Rectangle<int> dirtyArea) const;

    private:
        static constexpr char MARGIN = 6;
        static constexpr int CONTROL_HIT_TARGET_SIZE = 40;
        mpc::Mpc &mpcRef;
        AuxLcd *auxLcd = nullptr;
        AuxLcdWindowMaximizeButton maximizeButton;
        int buttonsHaveBeenShownForMs = 0;
        const std::function<void()> resetKeyboardAuxParent;
        const std::function<void()> resetAuxWindow;
        VmpcAuxLcdLookAndFeel lookAndFeel;
        juce::Colour backgroundColour;

        bool dragStarted = false;
        juce::ComponentDragger dragger;
        juce::ComponentBoundsConstrainer constrainer;

        std::unique_ptr<juce::ResizableCornerComponent> resizableCorner;

        void hideButtons();
        bool areButtonsShowing() const;
        void resetButtonShowTimer();
    };

    class MyResizableCornerComponent final
        : public juce::ResizableCornerComponent
    {
    public:
        MyResizableCornerComponent(
            Component *componentToResize,
            juce::ComponentBoundsConstrainer *constrainer)
            : ResizableCornerComponent(componentToResize, constrainer)
        {
        }
        void mouseEnter(const juce::MouseEvent &) override
        {
            dynamic_cast<AuxLcdWindow *>(getParentComponent())->showButtons();
        }
        void mouseDown(const juce::MouseEvent &e) override
        {
            dynamic_cast<AuxLcdWindow *>(getParentComponent())->showButtons();
            ResizableCornerComponent::mouseDown(e);
        }
        bool hitTest(int x, int y) override
        {
            return getLocalBounds().contains(x, y);
        }
    };
} // namespace vmpc_juce::gui
