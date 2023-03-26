#pragma once

#include "raw_keyboard_input/src/Keyboard.h"

class LCDControl;

class AuxLCD : public juce::Component
{
public:
    AuxLCD(LCDControl *lcdControlToUse, Keyboard *kb);

private:
    LCDControl *lcdControl;
    Keyboard *keyboard;

    bool keyPressed(const juce::KeyPress &) override;

    void resized() override;

    void mouseDoubleClick(const juce::MouseEvent &) override;

    void paint(juce::Graphics& g) override;
};
