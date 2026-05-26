#pragma once

namespace vmpc_juce::gui::vector::AnimatedButtonStyle
{

    inline constexpr float pressedOffsetY = 0.7f;
    inline constexpr float pressedShadowSizeMultiplier = 0.8f;
    inline constexpr float pressedShadowDarknessMultiplier = 0.8f;

    inline float getShadowSizeMultiplier(const bool pressed)
    {
        return pressed ? pressedShadowSizeMultiplier : 1.f;
    }

    inline float getShadowDarknessMultiplier(const bool pressed)
    {
        return pressed ? pressedShadowDarknessMultiplier : 1.f;
    }

    inline float getPressedOffsetY(const bool pressed)
    {
        return pressed ? pressedOffsetY : 0.f;
    }

} // namespace vmpc_juce::gui::vector::AnimatedButtonStyle
