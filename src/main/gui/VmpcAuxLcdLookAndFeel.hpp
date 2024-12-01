#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class VmpcAuxLcdLookAndFeel : public juce::LookAndFeel_V4 {

public:
    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
    static void drawLcdPixel(juce::Graphics& g, int x, int y);
    const static char LCD_PIXEL_SIZE = 2;
};