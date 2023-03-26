#include "AuxLCD.h"

#include "LCDControl.h"

AuxLCD::AuxLCD(LCDControl* lcdControlToUse, Keyboard *kb)
        : lcdControl(lcdControlToUse), keyboard(kb)
{
}

bool AuxLCD::keyPressed(const juce::KeyPress &)
{
    return true;
}

void AuxLCD::resized()
{
    const int margin = 0;
    setBounds(margin / 2, margin / 2, getParentWidth() - margin, getParentHeight() - margin);
}

void AuxLCD::mouseDoubleClick(const juce::MouseEvent &)
{
    keyboard->setAuxParent(nullptr);
    lcdControl->resetAuxWindow();
}

void AuxLCD::paint(juce::Graphics& g)
{
    g.drawImage(lcdControl->lcd, getLocalBounds().toFloat());
}
