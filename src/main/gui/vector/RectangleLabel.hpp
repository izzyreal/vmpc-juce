/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
#pragma once

#include "LabelComponent.hpp"

#include "SimpleLabel.hpp"

namespace vmpc_juce::gui::vector {
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
                    const std::function<juce::Font&()>& getScaledFont)
                : getScale(getScaleToUse),
                bgColour(bgColourToUse),
                textToCalculateWidth(textToCalculateWidthToUse),
                radius(radiusToUse),
                backgroundHorizontalMargin(backgroundHorizontalMarginToUse)
        {
            getFont = [getScaledFont, this]() -> juce::Font {
                auto font = getScaledFont();
                font.setHeight(font.getHeight() * fontScale);
                return font;
            };

            simpleLabel = new SimpleLabel(getScaleToUse, textToUse, fgColourToUse, getScaledFont);
            addAndMakeVisible(simpleLabel);
        }

            float getRequiredWidth() override
            {
                const auto requiredWidthOfText =
                    getFont().getStringWidth(textToCalculateWidth);
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

                const auto requiredWidth = getFont().getStringWidth(textToCalculateWidth);

                const auto amountToDeductFromWidth = ((float) (getWidth()) - (requiredWidth + backgroundHorizontalMargin)) / 2.f;
                const auto amountToDeductFromHeight = ((float) (getHeight()) - requiredHeight) / 2.f;
                backgroundRect.reduce(amountToDeductFromWidth, amountToDeductFromHeight);

                g.drawRoundedRectangle(backgroundRect.toFloat(), radiusWithScale, 1.f);
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
            SimpleLabel* simpleLabel = nullptr;
            const std::function<float()> &getScale;
            const juce::Colour bgColour;
            std::string textToCalculateWidth;
            const float radius;
            const float backgroundHorizontalMargin;
            std::function<juce::Font()> getFont;
    };
} // namespace vmpc_juce::gui::vector
