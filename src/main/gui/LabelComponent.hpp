#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class LabelComponent : public juce::Component {
    public:
        virtual float getRequiredWidth() = 0;
        virtual float getRequiredHeight() = 0;
};
