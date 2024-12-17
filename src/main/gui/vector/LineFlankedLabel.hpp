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
#include <juce_gui_basics/juce_gui_basics.h>

#include "SimpleLabel.hpp"
#include "Constants.hpp"
#include "juce_graphics/juce_graphics.h"

namespace vmpc_juce::gui::vector {

    class LineFlankedLabel : public juce::Component {
        public:
            LineFlankedLabel(const std::string &textToUse, const std::function<float()> &getScaleToUse,
                    const std::function<juce::Font&()> &getNimbusSansScaled)
                : text(textToUse), getScale(getScaleToUse)
            {
                simpleLabel = new SimpleLabel(getScaleToUse, textToUse, Constants::labelColour, getNimbusSansScaled);
                addAndMakeVisible(simpleLabel);
            }

            ~LineFlankedLabel() override
            {
                delete simpleLabel;
            }

            void resized() override
            {
                juce::FlexBox fb;
                fb.alignItems = juce::FlexBox::AlignItems::center;
                const auto requiredHeight = simpleLabel->getRequiredHeight();
                fb.items.add(juce::FlexItem(*simpleLabel).withFlex(1.f).withWidth(simpleLabel->getRequiredWidth()).withHeight(requiredHeight));
                fb.performLayout(getLocalBounds());
            }

            void paint(juce::Graphics& g) override
            {
                //g.fillAll(juce::Colours::yellowgreen);
                const auto textWidth = simpleLabel->getRequiredWidth() + (3.f * getScale());
                const auto lineInterruptionStartX = (getWidth() - textWidth) / 2;
                const auto lineInterruptionEndX = getWidth() - lineInterruptionStartX;

                const auto line_y_base = getHeight() / 2;
                const auto line_thickness = Constants::lineThickness1 * getScale();

                g.setColour(Constants::darkLabelColour);
                g.drawLine(0, line_y_base, lineInterruptionStartX, line_y_base, line_thickness);
                g.drawLine(lineInterruptionEndX, line_y_base, getWidth(), line_y_base, line_thickness);
            }

        private:
            const std::string text;
            const std::function<float()>& getScale;
            SimpleLabel* simpleLabel = nullptr;

    };

} // namespace vmpc_juce::gui::vector
