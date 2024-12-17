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

#include <juce_gui_basics/juce_gui_basics.h>

#include "SimpleLabel.hpp"
#include "Constants.hpp"
#include "SvgComponent.hpp"
#include "RectangleLabel.hpp"

namespace vmpc_juce::gui::vector {

    class NumKey : public juce::Component {
        public:
            NumKey(const std::function<float()> &getScaleToUse, const std::string topLabelToUse, const std::string bottomLabelToUse, std::string svgPath, juce::Component *commonParentWithShadow, const float shadowSize, const std::function<juce::Font&()> &getNimbusSansScaled)
            {
                topLabel = new SimpleLabel(getScaleToUse, topLabelToUse, Constants::darkLabelColour, getNimbusSansScaled);
                bottomLabel = new RectangleLabel(getScaleToUse, bottomLabelToUse, bottomLabelToUse, Constants::greyFacePaintColour, Constants::darkLabelColour, 0.5f, 2.f, getNimbusSansScaled);
                svgComponent = new SvgComponent({svgPath}, commonParentWithShadow, shadowSize, getScaleToUse);

                addAndMakeVisible(topLabel);
                addAndMakeVisible(bottomLabel);
                addAndMakeVisible(svgComponent);
            }

            ~NumKey() override
            {
                delete topLabel;
                delete bottomLabel;
                delete svgComponent;
            }

            void resized() override
            {
                juce::Grid grid;
                grid.templateRows = { juce::Grid::Fr(1), juce::Grid::Fr(1), juce::Grid::Fr(1) };
                grid.templateColumns = { juce::Grid::Fr(1) };

                grid.items.add(juce::GridItem(topLabel).withArea(1, 1, 1, 1));
                grid.items.add(juce::GridItem(svgComponent).withArea(2, 1, 2, 1));
                grid.items.add(juce::GridItem(bottomLabel).withArea(3, 1, 3, 1));

                grid.performLayout(getLocalBounds());
            }

            SvgComponent* getSvgComponent() { return svgComponent; }

        private:
            SimpleLabel* topLabel = nullptr;
            RectangleLabel* bottomLabel = nullptr;
            SvgComponent* svgComponent = nullptr;
    };

} // namespace vmpc_juce::gui::vector
