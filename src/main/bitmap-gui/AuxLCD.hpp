#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class LCDControl;

class AuxLCD : public juce::Component
{
public:
    explicit AuxLCD(LCDControl *lcdControlToUse);

private:
    LCDControl *lcdControl;

    void paint(juce::Graphics& g) override;
};
