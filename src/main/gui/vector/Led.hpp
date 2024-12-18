#pragma once

#include "SvgComponent.hpp"

namespace vmpc_juce::gui::vector {

    class Led : public SvgComponent {
        public:
            enum LedColor { RED, GREEN };

            Led(const std::string ledNameToUse, const LedColor ledColorToUse, const std::function<float()> &getScaleToUse)
                : SvgComponent({"led_off.svg", ledColorToUse == RED ? "led_on_red.svg" : "led_on_green.svg" }, nullptr, 0, getScaleToUse), ledColor(ledColorToUse), ledName(ledNameToUse)
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

                if (ledColor == RED)
                {
                    setSvgPath("led_on_red.svg");
                    return;
                }

                setSvgPath("led_on_green.svg");
            }

            const std::string getLedName() { return ledName; }

        private:
            const LedColor ledColor;
            bool ledOnEnabled = false;
            const std::string ledName;

    };

} // namespace vmpc_juce::gui::vector
