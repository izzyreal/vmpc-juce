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

#include "Constants.hpp"

namespace vmpc_juce::gui::vector {

    class JOrLShape : public juce::Component {

        public:
            enum Shape { J, L };

            JOrLShape(const Shape shapeToUse, const std::function<float()> &getScaleToUse)
                : shape(shapeToUse), getScale(getScaleToUse)
            {}

            void paint(juce::Graphics &g) override
            {
                g.setColour(Constants::darkLabelColour);

                const auto horizontal_line_thickness = Constants::lineThickness1 * getScale();
                const auto vertical_line_thickness = Constants::lineThickness2 * getScale();

                const auto half_thickness1 = Constants::lineThickness1 / 2;
                const auto half_thickness2 = Constants::lineThickness2 / 2;

                g.drawLine(0, getHeight() - std::ceil(half_thickness1), getWidth(), getHeight() - std::ceil(half_thickness1), horizontal_line_thickness);

                switch (shape)
                {
                    case Shape::J:
                        g.drawLine(getWidth() - half_thickness2, 0, getWidth() - half_thickness2, getHeight() - half_thickness1, vertical_line_thickness);
                        break;
                    case Shape::L:
                    default:
                        g.drawLine(0 + half_thickness2, 0, 0 + half_thickness2, getHeight() - half_thickness1, vertical_line_thickness);
                        break;
                }
            }

        private:
            const Shape shape;
            const std::function<float()> & getScale;
    };

} // namespace vmpc_juce::gui::vector
