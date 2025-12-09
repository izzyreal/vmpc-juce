#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/AuxLCD.hpp"
#include "gui/VmpcAuxLcdLookAndFeel.hpp"

namespace vmpc_juce::gui
{
    class AuxLCDWindowMaximizeButton final : public juce::Button
    {
    public:
        AuxLCDWindowMaximizeButton() : Button("MaximizeButton") {}
        void paint(juce::Graphics &g) override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseEnter(const juce::MouseEvent &e) override;

    protected:
        void paintButton(juce::Graphics &g,
                         bool /*shouldDrawButtonAsHighlighted*/,
                         bool /*shouldDrawButtonAsDown*/) override;
    };

    class AuxLCDWindow final : public juce::TopLevelWindow, public juce::Timer
    {
    public:
        explicit AuxLCDWindow(
            const std::function<void()> &resetAuxWindowToUse,
            const std::function<juce::Image &()> &getLcdImage,
            const std::function<void()> &resetKeyboardAuxParentToUse,
            juce::Colour backgroundColourToUse);

        void timerCallback() override;

        bool keyPressed(const juce::KeyPress &) override;

        void paint(juce::Graphics &) override;

        void resized() override;

        ~AuxLCDWindow() override;

        void mouseMove(const juce::MouseEvent &) override;

        void mouseEnter(const juce::MouseEvent &) override;

        void mouseDown(const juce::MouseEvent &) override;

        void mouseUp(const juce::MouseEvent &) override;

        void mouseDrag(const juce::MouseEvent &) override;

        void mouseDoubleClick(const juce::MouseEvent &) override;

        void showButtons();

        void repaintAuxLcdLocalBounds(juce::Rectangle<int> dirtyArea) const;

    private:
        static constexpr char MARGIN = 6;
        static constexpr unsigned char LCD_W = 248;
        static constexpr char LCD_H = 60;
        AuxLCD *auxLcd = nullptr;
        AuxLCDWindowMaximizeButton maximizeButton;
        Component maximizeButtonMouseInterceptor;
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
            dynamic_cast<AuxLCDWindow *>(getParentComponent())->showButtons();
        }
        void mouseDown(const juce::MouseEvent &e) override
        {
            dynamic_cast<AuxLCDWindow *>(getParentComponent())->showButtons();
            ResizableCornerComponent::mouseDown(e);
        }
    };
} // namespace vmpc_juce::gui
