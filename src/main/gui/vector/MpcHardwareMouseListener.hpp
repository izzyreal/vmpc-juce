#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Mpc.hpp"

#if __APPLE__
#include <TargetConditionals.h>
#endif

namespace vmpc_juce::gui::vector
{

    template <class ComponentClass>
    ComponentClass *getChildComponentOfClass(juce::Component *parent)
    {
        for (int i = 0; i < parent->getNumChildComponents(); ++i)
        {
            auto *childComp = parent->getChildComponent(i);

            if (auto c = dynamic_cast<ComponentClass *>(childComp))
            {
                return c;
            }

            if (auto c = getChildComponentOfClass<ComponentClass>(childComp))
            {
                return c;
            }
        }

        return nullptr;
    }

    class MpcHardwareMouseListener : public juce::MouseListener, juce::Timer
    {
    public:
        MpcHardwareMouseListener(mpc::Mpc &mpcToUse,
                                 const std::string labelToUse);
        void timerCallback() override;
        void mouseMove(const juce::MouseEvent &e) override;
        void mouseExit(const juce::MouseEvent &) override;
        void mouseWheelMove(const juce::MouseEvent &event,
                            const juce::MouseWheelDetails &) override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseDoubleClick(const juce::MouseEvent &) override;
        void mouseUp(const juce::MouseEvent &) override;
        void mouseDrag(const juce::MouseEvent &e) override;

    private:
        static bool showKeyTooltipUponNextClick;

        void setKeyTooltipVisibility(juce::Component *c,
                                     const bool visibleEnabled);
        bool isPad();

        mpc::Mpc &mpc;
        const std::string label;
        juce::Component *lastEventComponent = nullptr;
        bool hideKeyTooltipUntilAfterMouseExit = false;

        float previousDragY = 0.f;
    };

} // namespace vmpc_juce::gui::vector
