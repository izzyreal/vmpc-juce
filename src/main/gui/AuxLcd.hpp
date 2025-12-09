#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class AuxLcd : public juce::Component
{
public:
    explicit AuxLcd(const std::function<juce::Image &()> &getLcdImage);

private:
    const std::function<juce::Image &()> getLcdImage;

    void paint(juce::Graphics &g) override;
};
