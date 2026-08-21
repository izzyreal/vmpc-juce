#pragma once

#include "gui/arrangement/ArrangementModel.hpp"

#include <algorithm>

namespace vmpc_juce::gui::mobile
{
    inline constexpr int tabletMinimumShortestSide = 600;

    constexpr bool isPhoneSizedDisplay(const int width, const int height)
    {
        return width > 0 && height > 0 &&
               std::min(width, height) < tabletMinimumShortestSide;
    }

    bool isMobilePlatform();
    bool isPhone();
    void setOrientation(arrangement::Orientation orientation);
} // namespace vmpc_juce::gui::mobile
