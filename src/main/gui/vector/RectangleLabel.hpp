#pragma once

#include "LabelComponent.hpp"

#include "SimpleLabel.hpp"

#include "FloatUtil.hpp"

namespace vmpc_juce::gui::vector
{
    class RectangleLabel : public LabelComponent
    {
    public:
        RectangleLabel(const std::function<float()> &getScaleToUse,
                       std::string textToUse,
                       std::string textToCalculateWidthToUse,
                       juce::Colour bgColourToUse, juce::Colour fgColourToUse,
                       const float radiusToUse,
                       const float backgroundHorizontalMarginToUse,
                       const std::function<juce::Font &()> &getScaledFont,
                       const float topMarginToUse = -1)
            : getScale(getScaleToUse), bgColour(bgColourToUse),
              textToCalculateWidth(textToCalculateWidthToUse),
              radius(radiusToUse),
              backgroundHorizontalMargin(backgroundHorizontalMarginToUse),
              topMargin(topMarginToUse)
        {
            getFont = [getScaledFont, this]() -> juce::Font
            {
                auto font = getScaledFont();
                font.setHeight(font.getHeight() * fontScale);
                return font;
            };

            simpleLabel =
                new SimpleLabel(getScaleToUse, textToUse, fgColourToUse,
                                getScaledFont, topMargin);
            addAndMakeVisible(simpleLabel);
        }

        float getRequiredWidth() override
        {
            const auto requiredWidthOfText =
                getFont().getStringWidth(textToCalculateWidth);
            return static_cast<float>(requiredWidthOfText) +
                   backgroundHorizontalMargin;
        }

        float getRequiredHeight() override
        {
            return simpleLabel->getRequiredHeight();
        }

        void resized() override
        {
            simpleLabel->setBounds(getLocalBounds());
        }

        void paint(juce::Graphics &g) override
        {
            // g.fillAll(juce::Colours::yellow);
            g.setColour(bgColour);
            const auto scale = getScale();
            const auto radiusWithScale = radius * scale;
            const auto backgroundVerticalMargin = 0.01f;
            const auto requiredHeight =
                simpleLabel->getRequiredHeight() + backgroundVerticalMargin;
            auto backgroundRect = getLocalBounds();

            const auto requiredWidth =
                getFont().getStringWidth(textToCalculateWidth);

            const auto amountToDeductFromWidth =
                ((float)(getWidth()) - (static_cast<float>(requiredWidth) +
                                        backgroundHorizontalMargin)) /
                2.f;
            const auto amountToDeductFromHeight =
                ((float)(getHeight()) - requiredHeight) / 2.f;

            backgroundRect.reduce(static_cast<int>(amountToDeductFromWidth),
                                  static_cast<int>(amountToDeductFromHeight));

            if (!nearlyEqual(topMargin, -1.f))
            {
                backgroundRect.translate(
                    0, static_cast<int>(
                           (static_cast<float>(backgroundRect.getHeight()) -
                            getLocalBounds().toFloat().getHeight()) /
                           2.f));
                backgroundRect.translate(
                    0, static_cast<int>(topMargin * scale * 0.7f));
            }

            g.fillRoundedRectangle(backgroundRect.toFloat(), radiusWithScale);
        }

        void setFontScale(const float fontScaleToUse) override
        {
            simpleLabel->setFontScale(fontScaleToUse);
            LabelComponent::setFontScale(fontScaleToUse);
        }

        ~RectangleLabel() override
        {
            delete simpleLabel;
        }

    private:
        SimpleLabel *simpleLabel = nullptr;
        const std::function<float()> &getScale;
        const juce::Colour bgColour;
        std::string textToCalculateWidth;
        const float radius;
        const float backgroundHorizontalMargin;
        const float topMargin;
        std::function<juce::Font()> getFont;
    };
} // namespace vmpc_juce::gui::vector
