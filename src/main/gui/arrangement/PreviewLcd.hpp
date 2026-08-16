#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <lcdgui/LcdGeometry.hpp>
#include <melatonin_blur/melatonin/shadows.h>

#include <array>

namespace vmpc_juce::gui::arrangement
{
    class PreviewLcd final : public juce::Component
    {
    public:
        PreviewLcd();
        void paint(juce::Graphics &g) override;

        float magicMultiplier = 1.f;

    private:
        juce::Image pixels;
        std::array<std::array<bool, mpc::lcdgui::LCD_HEIGHT>,
                   mpc::lcdgui::LCD_WIDTH>
            pixelOn{};
        melatonin::DropShadow backlight;
    };
} // namespace vmpc_juce::gui::arrangement
