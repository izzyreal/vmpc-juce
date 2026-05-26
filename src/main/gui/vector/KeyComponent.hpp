#pragma once

#include "gui/WithSharedTimerCallback.hpp"
#include "SvgComponent.hpp"
#include "hardware/Component.hpp"

namespace vmpc_juce::gui::vector
{

    class KeyComponent final : public SvgComponent, public WithSharedTimerCallback
    {
    public:
        KeyComponent(const std::string &holeSvgPath,
                     const std::string &buttonSvgPath,
                     const std::shared_ptr<mpc::hardware::Button> &buttonToTrack,
                     juce::Component *commonParentWithShadowToUse,
                     const float shadowSizeToUse,
                     const std::function<float()> &getScaleToUse)
            : SvgComponent({holeSvgPath}, commonParentWithShadowToUse,
                           shadowSizeToUse, getScaleToUse),
              buttonSvg({buttonSvgPath}, nullptr, 0.f, getScaleToUse),
              trackedButton(buttonToTrack)
        {
        }

        juce::Rectangle<float> getDrawableBounds() override
        {
            return SvgComponent::getDrawableBounds();
        }

        juce::Path getShadowPath() override
        {
            return buttonSvg.getDrawablePathWithin(getButtonBounds());
        }

        float getShadowSizeMultiplier() const override
        {
            return pressedVisual ? 0.8f : 1.f;
        }

        float getShadowDarknessMultiplier() const override
        {
            return pressedVisual ? 0.8f : 1.f;
        }

        void paint(juce::Graphics &g) override
        {
            drawCurrentDrawableWithin(g, getLocalBounds().toFloat());
            buttonSvg.drawCurrentDrawableWithin(g, getButtonBounds());
        }

        void setPressedVisual(const bool shouldBePressed)
        {
            if (pressedVisual == shouldBePressed)
            {
                return;
            }

            pressedVisual = shouldBePressed;
            repaint();

            if (shadow != nullptr)
            {
                shadow->repaint();
            }
        }

        void sharedTimerCallback() override
        {
            if (trackedButton != nullptr)
            {
                setPressedVisual(trackedButton->isPressed());
            }
        }

    private:
        juce::Rectangle<float> getButtonBounds()
        {
            const auto holeBounds = SvgComponent::getDrawableBounds();
            const auto buttonBounds = buttonSvg.getDrawableBounds();

            if (holeBounds.isEmpty() || buttonBounds.isEmpty())
            {
                return getLocalBounds().toFloat();
            }

            const auto renderedHoleBounds =
                holeBounds.transformedBy(
                    getCurrentDrawableTransformWithin(getLocalBounds().toFloat()));

            const auto relativeWidth =
                buttonBounds.getWidth() / holeBounds.getWidth();
            const auto relativeHeight =
                buttonBounds.getHeight() / holeBounds.getHeight();

            const auto buttonWidth =
                relativeWidth * renderedHoleBounds.getWidth();
            const auto buttonHeight =
                relativeHeight * renderedHoleBounds.getHeight();

            return {renderedHoleBounds.getCentreX() - (buttonWidth * 0.5f),
                    renderedHoleBounds.getCentreY() - (buttonHeight * 0.5f) +
                        (pressedVisual ? 0.7f : 0.f),
                    buttonWidth, buttonHeight};
        }

        SvgComponent buttonSvg;
        std::shared_ptr<mpc::hardware::Button> trackedButton;
        bool pressedVisual = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyComponent)
    };

} // namespace vmpc_juce::gui::vector
