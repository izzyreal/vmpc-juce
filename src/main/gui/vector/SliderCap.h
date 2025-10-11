#pragma once

#include "DraggableSvgComponent.h"
#include "hardware2/HardwareComponent.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace vmpc_juce::gui::vector {

class SliderCap : public DraggableSvgComponent, private juce::Timer {
public:
    SliderCap(std::shared_ptr<mpc::hardware2::Slider> modelToUse,
              const std::vector<std::string>& svgPaths,
              juce::Component* commonParentWithShadowToUse,
              float shadowSizeToUse,
              const std::function<float()>& getScaleToUse)
        : DraggableSvgComponent(svgPaths, commonParentWithShadowToUse, shadowSizeToUse, getScaleToUse),
          model(modelToUse)
    {
        startTimerHz(30); // poll ~33 ms
    }

private:
    std::shared_ptr<mpc::hardware2::Slider> model;
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
        model->moveToNormalizedY(getNormalizedY());
    }

    void timerCallback() override
    {
        if (isDragging)
        {
            return;
        }

        const auto [min, max] = model->getRangeAs<float>();
        const float modelValue = model->getValue();
        float norm = std::clamp((modelValue - min) / (max - min), 0.0f, 1.0f);

        if (model->getDirection() == mpc::hardware2::Slider::Direction::UpIncreases)
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
};

} // namespace vmpc_juce::gui::vector
