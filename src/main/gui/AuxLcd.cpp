#include "AuxLcd.hpp"

AuxLcd::AuxLcd(const std::function<juce::Image &()> &getLcdImageToUse)
    : Component("auxlcd"), getLcdImage(getLcdImageToUse)
{
    setInterceptsMouseClicks(false, false);
}

void AuxLcd::paint(juce::Graphics &g)
{
    g.setImageResamplingQuality(g.lowResamplingQuality);
    g.drawImage(getLcdImage(), getLocalBounds().toFloat());
}
