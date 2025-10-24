#pragma once

#include "DraggableSvgComponent.hpp"
#include "hardware/Component.hpp"
#include "input/HostInputEvent.hpp"
#include <juce_gui_basics/juce_gui_basics.h>

#include "MpcInputUtil.hpp"
#include "Mpc.hpp"

namespace vmpc_juce::gui::vector {

class SliderCap : public DraggableSvgComponent {
public:
    SliderCap(mpc::Mpc &mpcToUse, std::shared_ptr<mpc::hardware::Slider> modelToUse,
              const std::vector<std::string>& svgPaths,
              juce::Component* commonParentWithShadowToUse,
              float shadowSizeToUse,
              const std::function<float()>& getScaleToUse)
        : DraggableSvgComponent(svgPaths, commonParentWithShadowToUse, shadowSizeToUse, getScaleToUse),
          mpc(mpcToUse), model(modelToUse)
    {
    }

    void sharedTimerCallback()
    {
        if (isDragging)
        {
            return;
        }

        const auto [min, max] = model->getRangeAs<float>();
        const float modelValue = model->getValue();
        float norm = std::clamp((modelValue - min) / (max - min), 0.0f, 1.0f);

        if (model->getDirection() == mpc::hardware::Slider::Direction::UpIncreases)
        {
            norm = 1.f - norm;
        }

        if (const auto *parent = getParentComponent())
        {
            const float range = static_cast<float>(parent->getHeight() - getHeight());
            const int newY = static_cast<int>(std::lround(range * norm));
            setTopLeftPosition(getX(), newY);
        }
    }

private:
    mpc::Mpc &mpc;
    std::shared_ptr<mpc::hardware::Slider> model;
    bool isDragging = false;

    void mouseDown(const juce::MouseEvent& e) override
    {
        isDragging = true;
        DraggableSvgComponent::mouseDown(e);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        isDragging = false;
        DraggableSvgComponent::mouseUp(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        DraggableSvgComponent::mouseDrag(e);
        if (auto hostInputEvent = makeAbsoluteGestureFromMouse(e, "slider", mpc::input::GestureEvent::Type::UPDATE,
                                                                            getNormalizedY()); hostInputEvent) 
        {
            mpc.dispatchHostInput(*hostInputEvent);
        }
    }
};

} // namespace vmpc_juce::gui::vector
