#pragma once

#include "SvgComponent.hpp"

namespace vmpc_juce::gui::vector {

    class Led : public SvgComponent {
        public:
            enum LedColor { RED, GREEN };

            Led(const LedColor ledColorToUse, const std::function<float()> &getScaleToUse) : SvgComponent("led_off.svg", nullptr, 0, getScaleToUse), ledColor(ledColorToUse)
        {
        }

            void setLedOnEnabled(const bool b)
            {
                if (ledOnEnabled == b) return;

                ledOnEnabled = b;

                if (!ledOnEnabled)
                {
                    setSvgPath("led_off.svg");
                    return;
                }

                if (ledColor == LedColor::RED)
                {
                    setSvgPath("led_on_red.svg");
                    return;
                }

                setSvgPath("led_on_green.svg");
            }

        private:
            const LedColor ledColor;
            bool ledOnEnabled = false;

    };

} // namespace vmpc_juce::gui::vector
