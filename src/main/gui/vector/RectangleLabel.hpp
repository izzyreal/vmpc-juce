#pragma once

#include "LabelComponent.hpp"

#include "SimpleLabel.hpp"

class RectangleLabel : public LabelComponent {
    public:
        RectangleLabel(
                const std::function<float()>& getScaleToUse,
                std::string textToUse,
                std::string textToCalculateWidthToUse,
                juce::Colour bgColourToUse,
                juce::Colour fgColourToUse,
                const float radiusToUse,
                const float backgroundHorizontalMarginToUse,
                const std::function<juce::Font&()>& getNimbusSansScaledToUse)
            : getScale(getScaleToUse),
            bgColour(bgColourToUse),
            textToCalculateWidth(textToCalculateWidthToUse),
            radius(radiusToUse),
            backgroundHorizontalMargin(backgroundHorizontalMarginToUse),
            getNimbusSansScaled(getNimbusSansScaledToUse)
        {
            simpleLabel = new SimpleLabel(getScaleToUse, textToUse, fgColourToUse, getNimbusSansScaled);
            addAndMakeVisible(simpleLabel);
        }

        float getRequiredWidth() override
        {
            const auto requiredWidthOfText =
                getNimbusSansScaled().getStringWidth(textToCalculateWidth);
            return requiredWidthOfText + backgroundHorizontalMargin;
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
            //g.fillAll(juce::Colours::yellow);
            g.setColour(bgColour);
            const auto radiusWithScale = radius * getScale();
            const auto backgroundVerticalMargin = 0.01f;
            const auto requiredHeight = simpleLabel->getRequiredHeight() + backgroundVerticalMargin;
            auto backgroundRect = getLocalBounds();

            const auto requiredWidth = getNimbusSansScaled().getStringWidth(textToCalculateWidth);

            const auto amountToDeductFromWidth = ((float) (getWidth()) - (requiredWidth + backgroundHorizontalMargin)) / 2.f;
            const auto amountToDeductFromHeight = ((float) (getHeight()) - requiredHeight) / 2.f;
            backgroundRect.reduce(amountToDeductFromWidth, amountToDeductFromHeight);

            g.drawRoundedRectangle(backgroundRect.toFloat(), radiusWithScale, 1.f);
            g.fillRoundedRectangle(backgroundRect.toFloat(), radiusWithScale);

            LabelComponent::paint(g);
        }

        ~RectangleLabel() override
        {
            delete simpleLabel;
        }

    private:
        SimpleLabel* simpleLabel = nullptr;
        const std::function<float()> &getScale;
        const juce::Colour bgColour;
        std::string textToCalculateWidth;
        const float radius;
        const float backgroundHorizontalMargin;
        const std::function<juce::Font&()> &getNimbusSansScaled;
};

