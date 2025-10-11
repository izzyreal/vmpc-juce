#pragma once

#include "SvgComponent.hpp"

#include "hardware2/HardwareComponent.h"

namespace vmpc_juce::gui::vector {

    class Led : public SvgComponent {
        public:
            enum LedColor { RED, GREEN };

            Led(const std::shared_ptr<mpc::hardware2::Led> mpcLedToUse,
                    const LedColor ledColorToUse,
                    const std::function<float()> &getScaleToUse)
                : SvgComponent({"led_off.svg", ledColorToUse == RED ? "led_on_red.svg" : "led_on_green.svg" },
                               nullptr,
                               0,
                               getScaleToUse),
                ledColor(ledColorToUse),
                mpcLed(mpcLedToUse)
                {
                    setLedOnEnabled(true);
                }

            void sharedTimerCallback() { setLedOnEnabled(mpcLed->isEnabled()); }

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

        private:
            const LedColor ledColor;
            bool ledOnEnabled = false;
            const std::shared_ptr<mpc::hardware2::Led> mpcLed;
    };

} // namespace vmpc_juce::gui::vector
