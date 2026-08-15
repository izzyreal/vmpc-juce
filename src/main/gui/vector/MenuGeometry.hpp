#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector::menu_geometry
{

    constexpr float heightAtScale1 = 16.f * 1.1f;

    inline juce::Rectangle<float>
    getBackgroundBounds(const juce::Rectangle<int> visibleIconBounds,
                        const float componentHeight, const float scale,
                        const bool expandedOrHovered)
    {
        const auto lineThickness = scale;
        constexpr auto marginAtScale1 = 5.f;
        const auto fixedHeight = heightAtScale1 * scale - lineThickness * 3.f;
        const auto centerY = (componentHeight - fixedHeight) * 0.5f;

        auto result = visibleIconBounds.toFloat().expanded(
            marginAtScale1 * scale, lineThickness);
        result.setY(centerY);
        result.setHeight(fixedHeight);
        result = result.withTrimmedRight(lineThickness * 2.f);

        if (!expandedOrHovered)
        {
            result = result.withTrimmedLeft(lineThickness * 2.f);
        }

        return result;
    }

    inline juce::Rectangle<float>
    getInteractiveBounds(const juce::Rectangle<int> visibleIconBounds,
                         const float componentHeight, const float scale,
                         const bool expandedOrHovered)
    {
        const auto background = getBackgroundBounds(
            visibleIconBounds, componentHeight, scale, expandedOrHovered);
        const auto iconTouchBounds =
            visibleIconBounds.toFloat().expanded(scale * 3.f);
        return background.getUnion(iconTouchBounds);
    }

} // namespace vmpc_juce::gui::vector::menu_geometry
