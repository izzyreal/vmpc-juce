#include "AuxLCD.h"

#include "LCDControl.h"

AuxLCD::AuxLCD(LCDControl* lcdControlToUse)
        : Component("auxlcd"), lcdControl(lcdControlToUse)
{
    setWantsKeyboardFocus(false);
    setInterceptsMouseClicks(false, false);
}

void AuxLCD::resized()
{
    const int margin = 0;
    setBounds(margin / 2, margin / 2, getParentWidth() - margin, getParentHeight() - margin);
}

void AuxLCD::paint(juce::Graphics& g)
{
    g.drawImage(lcdControl->lcd, getLocalBounds().toFloat());
}
