#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class Rectangle : public juce::Component {
    public:
        Rectangle(const juce::Colour colourToUse) : colour(colourToUse) {}

        void paint(juce::Graphics &g) override
        {
            g.fillAll(colour);
        }

    private:
        juce::Colour colour;
};
