#include "AuxLCD.h"

#include "LCDControl.h"

AuxLCD::AuxLCD(LCDControl* lcdControlToUse)
        : Component("auxlcd"), lcdControl(lcdControlToUse)
{
    setWantsKeyboardFocus(false);
    setInterceptsMouseClicks(false, false);
}

void AuxLCD::paint(juce::Graphics& g)
{
    g.setImageResamplingQuality(g.lowResamplingQuality);
    g.drawImage(lcdControl->lcd, getLocalBounds().toFloat());
}
