#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "LabelComponent.hpp"
#include "Constants.hpp"

#include "FloatUtil.hpp"

#include <string>
#include <functional>

namespace vmpc_juce::gui::vector {
    class SimpleLabel : public LabelComponent {
        public:
            SimpleLabel(
                    const std::function<float()>& getScaleToUse,
                    const std::string textToUse,
                    const juce::Colour colourToUse,
                    const std::function<juce::Font&()>& getScaledFont,
                    const float topMarginToUse = -1.f)
                : text(textToUse), getScale(getScaleToUse), colour(colourToUse), topMargin(topMarginToUse)
            {
                getFont = [getScaledFont, this]() -> juce::Font {
                    auto font = getScaledFont();
                    font.setHeight(font.getHeight() * fontScale);
                    return font;
                };
            }

            void paint(juce::Graphics& g) override
            {
                //g.fillAll(juce::Colours::yellowgreen);

                g.setFont(getFont());

                int row = 0;
                std::string buf;
                bool should_draw = false;

                g.setColour(colour);
                const auto yOffset = !nearlyEqual(topMargin, -1.f) ? topMargin : std::floor((static_cast<float>(getHeight()) - getRequiredHeight()) / 2.f);

                for (auto c : text) 
                {
                    if (c == '\n')
                    {
                        g.drawText(buf, 0, static_cast<int>(std::round(yOffset + static_cast<float>(row) * (Constants::BASE_FONT_SIZE + Constants::LINE_SIZE) * getScale())), getWidth(), getHeight(), juce::Justification::centredTop);
                        row++;
                        should_draw = false;
                        buf.clear();
                        continue;
                    }
                    buf += c;
                    should_draw = true;
                }

                if (should_draw && !buf.empty())
                {
                    g.drawText(buf, 0, static_cast<int>(std::round(yOffset + static_cast<float>(row) * (Constants::BASE_FONT_SIZE + Constants::LINE_SIZE) * getScale())), getWidth(), getHeight(), juce::Justification::centredTop);
                }
            }

            float getRequiredWidth() override
            {
                int upper = 0;

                std::string buf;
                bool should_check = true;

                auto font = getFont();

                for (auto c : text)
                {
                    if (c == '\n')  
                    {
                        upper = std::max<int>(font.getStringWidth(buf), upper);
                        buf.clear();
                        should_check = false;
                        continue;
                    }

                    buf += c;
                    should_check = true;
                }

                if (should_check && !buf.empty())
                {
                    upper = std::max<int>(font.getStringWidth(buf), upper);
                }

                return ((float) upper );
            }

            float getRequiredHeight() override
            {
                const auto newlineCount = (float) std::count(text.begin(), text.end(), '\n');
                const auto labelHeight = ((Constants::BASE_FONT_SIZE * (newlineCount + 1)) + (Constants::LINE_SIZE * newlineCount)) * getScale() * fontScale;
                return labelHeight;
            }

        private:
            std::string text;
            const std::function<float()> &getScale;
            juce::Colour colour;
            std::function<juce::Font()> getFont;
            const float topMargin;
    };
} // namespace vmpc_juce::gui::vector
