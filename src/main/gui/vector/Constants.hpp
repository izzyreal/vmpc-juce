#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector
{

    class Constants
    {
    public:
        static juce::Colour chassisColour;
        static juce::Colour labelColour;
        static juce::Colour darkLabelColour;
        static juce::Colour greyFacePaintColour;
        static juce::Colour betweenChassisAndLabelColour;

        static juce::Colour lcdOn;
        static juce::Colour lcdOnLight;
        static juce::Colour lcdOff;
        static juce::Colour lcdOffBacklit;

        static float BASE_FONT_SIZE;
        static float LINE_SIZE;
        static float lineThickness1;
        static float lineThickness2;
    };

} // namespace vmpc_juce::gui::vector
