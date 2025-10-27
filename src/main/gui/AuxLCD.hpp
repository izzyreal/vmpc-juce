#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class AuxLCD : public juce::Component
{
public:
    explicit AuxLCD(const std::function<juce::Image &()> &getLcdImage);

private:
    const std::function<juce::Image &()> getLcdImage;

    void paint(juce::Graphics &g) override;
};
