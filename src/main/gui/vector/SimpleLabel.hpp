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

#include "juce_graphics/juce_graphics.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include "LabelComponent.hpp"
#include "Constants.hpp"

#include <string>
#include <functional>

namespace vmpc_juce::gui::vector {
    class SimpleLabel : public LabelComponent {
        public:
            SimpleLabel(
                    const std::function<float()>& getScaleToUse,
                    const std::string textToUse,
                    const juce::Colour colourToUse,
                    const std::function<juce::Font&()>& getScaledFont)
                : text(textToUse), getScale(getScaleToUse), colour(colourToUse)
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
                const auto yOffset = (getHeight() - getRequiredHeight()) / 2;

                for (auto c : text) 
                {
                    if (c == '\n')
                    {
                        g.drawText(buf, 0, std::round(yOffset + row * (Constants::BASE_FONT_SIZE + Constants::LINE_SIZE) * getScale()), getWidth(), getHeight(), juce::Justification::centredTop);
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
                    g.drawText(buf, 0, std::round(yOffset + row * (Constants::BASE_FONT_SIZE + Constants::LINE_SIZE) * getScale()), getWidth(), getHeight(), juce::Justification::centredTop);
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
    };
} // namespace vmpc_juce::gui::vector
