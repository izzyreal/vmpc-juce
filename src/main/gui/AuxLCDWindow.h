#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AuxLCD.h"

class Keyboard;
class LCDControl;

class AuxLCDWindow : public juce::TopLevelWindow
{

public:
    AuxLCDWindow(LCDControl*, Keyboard*);

    bool keyPressed(const juce::KeyPress& e) override;

    void paint(juce::Graphics&) override;

    void resized() override;

    ~AuxLCDWindow() override;

    void mouseDown (const juce::MouseEvent& e) override;

    void mouseDrag (const juce::MouseEvent& e) override;

    void mouseDoubleClick(const juce::MouseEvent &) override;

private:
    AuxLCD* auxLcd = nullptr;
    Keyboard *keyboard;
    LCDControl *lcdControl;

    bool dragStarted = false;
    juce::ComponentDragger dragger;
    juce::ComponentBoundsConstrainer constrainer;

    std::unique_ptr<juce::ResizableCornerComponent> resizableCorner;
    std::unique_ptr<juce::ResizableBorderComponent> resizableBorder;

    void setBoundsConstrained (const juce::Rectangle<int>& newBounds);
    void setResizable (const bool shouldBeResizable,
                                        const bool useBottomRightCornerResizer);

    void setResizeLimits (int newMinimumWidth,
                                           int newMinimumHeight,
                                           int newMaximumWidth,
                                           int newMaximumHeight) noexcept;
};
