#pragma once

#include "SvgComponent.hpp"

#include "controller/ClientHardwareControllerBase.hpp"
#include "hardware/Component.hpp"

namespace vmpc_juce::gui::vector {

    class Led : public SvgComponent {
        public:
            enum LedColor { RED, GREEN };

            Led(const std::shared_ptr<mpc::hardware::Led> mpcLedToUse,
                    std::shared_ptr<mpc::controller::ClientHardwareControllerBase> clientHardwareControllerToUse,
                    const LedColor ledColorToUse,
                    const std::function<float()> &getScaleToUse)
                : SvgComponent({"led_off.svg", ledColorToUse == RED ? "led_on_red.svg" : "led_on_green.svg" },
                               nullptr,
                               0,
                               getScaleToUse),
                ledColor(ledColorToUse),
                mpcLed(mpcLedToUse),
                clientHardwareController(clientHardwareControllerToUse)
                {
                    setLedOnEnabled(true);
                }

            void sharedTimerCallback()
            {
                using namespace mpc::hardware;
                
                if (mpcLed->getId() == ComponentId::REC_LED)
                {
                    setLedOnEnabled(mpcLed->isEnabled() || clientHardwareController->buttonLockTracker.isLocked(ComponentId::REC));
                    return;
                }
                if (mpcLed->getId() == ComponentId::OVERDUB_LED)
                {
                    setLedOnEnabled(mpcLed->isEnabled() || clientHardwareController->buttonLockTracker.isLocked(ComponentId::OVERDUB));
                    return;
                }

                setLedOnEnabled(mpcLed->isEnabled());
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

        private:
            const LedColor ledColor;
            bool ledOnEnabled = false;
            const std::shared_ptr<mpc::hardware::Led> mpcLed;
            const std::shared_ptr<mpc::controller::ClientHardwareControllerBase> clientHardwareController;
    };

} // namespace vmpc_juce::gui::vector
