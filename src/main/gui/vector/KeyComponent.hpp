#pragma once

#include "SvgComponent.hpp"

namespace vmpc_juce::gui::vector
{

    class KeyComponent final : public SvgComponent
    {
    public:
        KeyComponent(const std::string &holeSvgPath,
                     const std::string &buttonSvgPath,
                     juce::Component *commonParentWithShadowToUse,
                     const float shadowSizeToUse,
                     const std::function<float()> &getScaleToUse)
            : SvgComponent({holeSvgPath}, commonParentWithShadowToUse,
                           shadowSizeToUse, getScaleToUse),
              buttonSvg({buttonSvgPath}, nullptr, 0.f, getScaleToUse)
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

        void paint(juce::Graphics &g) override
        {
            drawCurrentDrawableWithin(g, getLocalBounds().toFloat());
            buttonSvg.drawCurrentDrawableWithin(g, getButtonBounds());
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
                    renderedHoleBounds.getCentreY() - (buttonHeight * 0.5f),
                    buttonWidth, buttonHeight};
        }

        SvgComponent buttonSvg;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyComponent)
    };

} // namespace vmpc_juce::gui::vector
