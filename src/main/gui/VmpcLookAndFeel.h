#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class VmpcLookAndFeel : public juce::LookAndFeel_V4 {

public:
    void drawCornerResizer (juce::Graphics&, int, int, bool, bool) override {}
};