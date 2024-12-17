/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/AuxLCD.hpp"
#include "gui/VmpcAuxLcdLookAndFeel.hpp"

namespace vmpc_juce::gui {

class AuxLCDWindowMaximizeButton : public juce::Button {
public:
    AuxLCDWindowMaximizeButton() : juce::Button("MaximizeButton") {}
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;

protected:
    void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class AuxLCDWindow : public juce::TopLevelWindow, public juce::Timer
{
public:
    explicit AuxLCDWindow(const std::function<void()> &resetAuxWindow, const std::function<juce::Image&()> &getLcdImage, const std::function<void()> &resetKeyboardAuxParent, const juce::Colour backgroundColourToUse);

    void timerCallback() override;

    bool keyPressed(const juce::KeyPress& e) override;

    void paint(juce::Graphics&) override;

    void resized() override;

    ~AuxLCDWindow() override;

    void mouseMove(const juce::MouseEvent& e) override;

    void mouseEnter(const juce::MouseEvent& e) override;

    void mouseDown(const juce::MouseEvent& e) override;

    void mouseUp(const juce::MouseEvent& e) override;

    void mouseDrag(const juce::MouseEvent& e) override;

    void mouseDoubleClick(const juce::MouseEvent &) override;

    void showButtons();

    void repaintAuxLcdLocalBounds(juce::Rectangle<int> dirtyArea);

private:
    static const char MARGIN = 6;
    static const unsigned char LCD_W = 248;
    static const char LCD_H = 60;
    AuxLCD* auxLcd = nullptr;
    AuxLCDWindowMaximizeButton maximizeButton;
    juce::Component maximizeButtonMouseInterceptor;
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
    bool areButtonsShowing();
    void resetButtonShowTimer();

    void setBoundsConstrained(const juce::Rectangle<int>& newBounds);

    void setResizeLimits(int newMinimumWidth,
                                           int newMinimumHeight,
                                           int newMaximumWidth,
                                           int newMaximumHeight) noexcept;

};

class MyResizableCornerComponent : public juce::ResizableCornerComponent {
public:
    MyResizableCornerComponent(juce::Component* componentToResize, juce::ComponentBoundsConstrainer* constrainer)
    : juce::ResizableCornerComponent(componentToResize, constrainer) {}
    void mouseEnter(const juce::MouseEvent&) override {
        dynamic_cast<AuxLCDWindow*>(getParentComponent())->showButtons();
    }
    void mouseDown(const juce::MouseEvent & e) override {
        dynamic_cast<AuxLCDWindow*>(getParentComponent())->showButtons();
        ResizableCornerComponent::mouseDown(e);
    }
};
} //namespace vmpc_juce::gui
