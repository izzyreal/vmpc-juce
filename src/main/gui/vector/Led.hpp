#pragma once

#include "SvgComponent.hpp"

#include "controller/ClientEventController.hpp"
#include "controller/ClientHardwareEventController.hpp"
#include "hardware/Component.hpp"
#include "sequencer/Transport.hpp"

namespace vmpc_juce::gui::vector
{
    class Led : public SvgComponent
    {
    public:
        enum LedColor
        {
            RED,
            GREEN
        };

        Led(const std::shared_ptr<mpc::hardware::Led> &mpcLedToUse,
            const std::function<std::shared_ptr<mpc::sequencer::Transport>()> &getTransportToUse,
            const std::shared_ptr<mpc::controller::ClientEventController>
                &clientEventControllerToUse,
            const LedColor ledColorToUse,
            const std::function<float()> &getScaleToUse)
            : SvgComponent({"led_off.svg", ledColorToUse == RED
                                               ? "led_on_red.svg"
                                               : "led_on_green.svg"},
                           nullptr, 0, getScaleToUse), ledColor(ledColorToUse),
              getTransport(getTransportToUse), mpcLed(mpcLedToUse),
              clientEventController(clientEventControllerToUse)
        {
            setLedOnEnabled(true);
        }

        void sharedTimerCallback()
        {
            using namespace mpc::hardware;

            if (mpcLed->getId() == REC_LED)
            {
                setLedOnEnabled(
                    mpcLed->isEnabled() || getTransport()->isRecording() ||
                    clientEventController->clientHardwareEventController
                        ->buttonLockTracker.isLocked(REC));
                return;
            }
            if (mpcLed->getId() == OVERDUB_LED)
            {
                setLedOnEnabled(
                    mpcLed->isEnabled() || getTransport()->isOverdubbing() ||
                    clientEventController->clientHardwareEventController
                        ->buttonLockTracker.isLocked(OVERDUB));
                return;
            }
            if (mpcLed->getId() == PLAY_LED) {
                setLedOnEnabled(
                    getTransport()->isPlaying());
                return;
            }

            setLedOnEnabled(mpcLed->isEnabled());
        }

        void setLedOnEnabled(const bool b)
        {
            if (ledOnEnabled == b)
            {
                return;
            }

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
        std::function<std::shared_ptr<mpc::sequencer::Transport>()> getTransport;
        const std::shared_ptr<mpc::hardware::Led> mpcLed;
        const std::shared_ptr<mpc::controller::ClientEventController>
            clientEventController;
    };
} // namespace vmpc_juce::gui::vector
