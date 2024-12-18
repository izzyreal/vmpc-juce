#include "AuxLCD.hpp"

AuxLCD::AuxLCD(const std::function<juce::Image&()> &getLcdImageToUse)
        : Component("auxlcd"), getLcdImage(getLcdImageToUse)
{
    setWantsKeyboardFocus(false);
    setInterceptsMouseClicks(false, false);
}

void AuxLCD::paint(juce::Graphics& g)
{
    g.setImageResamplingQuality(g.lowResamplingQuality);
    g.drawImage(getLcdImage(), getLocalBounds().toFloat());
}
