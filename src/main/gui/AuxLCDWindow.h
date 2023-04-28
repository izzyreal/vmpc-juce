#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AuxLCD.h"
#include "VmpcLookAndFeel.h"

class Keyboard;
class LCDControl;

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
    AuxLCDWindow(LCDControl*, Keyboard*);

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

private:
    static const char MARGIN = 6;
    static const unsigned char LCD_W = 248;
    static const char LCD_H = 60;
    AuxLCD* auxLcd = nullptr;
    AuxLCDWindowMaximizeButton maximizeButton;
    juce::Component maximizeButtonMouseInterceptor;
    int buttonsHaveBeenShownForMs = 0;
    Keyboard *keyboard;
    LCDControl *lcdControl;
    VmpcLookAndFeel lookAndFeel;

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

    friend class LCDControl;
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

