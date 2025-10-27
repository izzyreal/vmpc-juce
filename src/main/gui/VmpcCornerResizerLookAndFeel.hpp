#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

class VmpcCornerResizerLookAndFeel : public juce::LookAndFeel_V4
{

public:
    void drawCornerResizer(juce::Graphics &g, int w, int h, bool isMouseOver,
                           bool isMouseDragging) override
    {
        const auto color1 = juce::Colours::transparentBlack;
        const auto color2 = juce::Colours::black.withAlpha(
            isMouseOver || isMouseDragging ? 1.f : 0.5f);

        const juce::ColourGradient gradient(color1, w / 2, h / 2, color2, w, h,
                                            false);

        juce::Path path;
        path.addTriangle(0, h, w, h, w, 0);

        g.setGradientFill(gradient);
        g.fillPath(path);
    }
};
