#pragma once

#include <Observer.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace vmpc_juce::gui::vector
{
    class Pad;

    class PadTimer final : public juce::Timer, public mpc::Observer
    {
    public:
        explicit PadTimer(const std::vector<Pad *> &);

        ~PadTimer() override;

        void timerCallback() override;

    private:
        std::vector<Pad *> pads;
    };
} // namespace vmpc_juce::gui::vector