#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui
{
    inline float mouseWheelContinuousDelta(const juce::MouseWheelDetails &wheel)
    {
        float sensitivity = 10.f;

        if (wheel.isSmooth)
        {
            sensitivity *= 4.f;
        }

        if (wheel.isInertial)
        {
            sensitivity *= 2.f;
        }

        return -wheel.deltaY * sensitivity;
    }
} // namespace vmpc_juce::gui
