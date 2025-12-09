#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Mpc.hpp"

namespace vmpc_juce::gui::vector
{
    class MpcHardwareMouseListener final : public juce::MouseListener,
                                           juce::Timer
    {
    public:
        MpcHardwareMouseListener(mpc::Mpc &mpcToUse,
                                 const std::string &labelToUse);
        void timerCallback() override;
        void mouseMove(const juce::MouseEvent &) override;
        void mouseExit(const juce::MouseEvent &) override;
        void mouseWheelMove(const juce::MouseEvent &,
                            const juce::MouseWheelDetails &) override;
        void mouseDown(const juce::MouseEvent &) override;
        void mouseDoubleClick(const juce::MouseEvent &) override;
        void mouseUp(const juce::MouseEvent &) override;
        void mouseDrag(const juce::MouseEvent &) override;

    private:
        static bool showKeyTooltipUponNextClick;

        void setKeyTooltipVisibility(const juce::Component *c,
                                     bool visibleEnabled) const;
        bool isPad() const;

        mpc::Mpc &mpc;
        const std::string label;
        juce::Component *lastEventComponent = nullptr;
        bool hideKeyTooltipUntilAfterMouseExit = false;

        using MouseSourceIndex = int;

        std::unordered_map<MouseSourceIndex, float> previousDragY;
    };

} // namespace vmpc_juce::gui::vector
