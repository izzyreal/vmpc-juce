#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Mpc.hpp"
#include "hardware/Hardware.hpp"
#include "hardware/Button.hpp"
#include "hardware/HwPad.hpp"
#include "hardware/DataWheel.hpp"
#include "hardware/Pot.hpp"
#include "hardware/HwSlider.hpp"
#include "controls/KbMapping.hpp"

#if __APPLE__
#include <TargetConditionals.h>
#endif

namespace vmpc_juce::gui::vector {

    template<class ComponentClass>
    static ComponentClass* getChildComponentOfClass(juce::Component *parent);

    class MpcHardwareMouseListener : public juce::MouseListener, juce::Timer
    {
        public:
            MpcHardwareMouseListener(mpc::Mpc &mpcToUse, const std::string labelToUse);
            void timerCallback() override;
            void mouseMove(const juce::MouseEvent &e) override;
            void mouseExit(const juce::MouseEvent &) override;
            void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &) override;
            void mouseDown(const juce::MouseEvent &e) override;
            void mouseDoubleClick(const juce::MouseEvent &) override;
            void mouseUp(const juce::MouseEvent &) override;
            void mouseDrag(const juce::MouseEvent &e) override;

        private:
            static bool showKeyTooltipUponNextClick;

            void pushHardware(const juce::MouseEvent &e);
            void setKeyTooltipVisibility(juce::Component *c, const bool visibleEnabled);
            void syncMpcSliderModelWithUi(juce::Component *eventComponent);
            bool isPad();

            mpc::Mpc &mpc;
            const std::string label;
            juce::Component *lastEventComponent = nullptr;
            bool hideKeyTooltipUntilAfterMouseExit = false;
    };

} // namespace vmpc_juce::gui::vector
