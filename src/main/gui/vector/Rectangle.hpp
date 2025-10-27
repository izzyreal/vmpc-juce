#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector
{
    class Rectangle : public juce::Component
    {
    public:
        Rectangle(const juce::Colour colourToUse) : colour(colourToUse) {}

        void paint(juce::Graphics &g) override
        {
            g.setColour(colour);
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 1.f);
        }

    private:
        juce::Colour colour;
    };
} // namespace vmpc_juce::gui::vector
