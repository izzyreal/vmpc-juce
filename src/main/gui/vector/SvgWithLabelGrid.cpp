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
#include "SvgWithLabelGrid.hpp"

#include "ViewUtil.hpp"
#include "SvgComponent.hpp"

using namespace vmpc_juce::gui::vector;

SvgWithLabelGrid::SvgWithLabelGrid(const struct node &nodeToUse, const std::function<float()>& getScaleToUse)
    : node(nodeToUse), getScale(getScaleToUse)
{
    addAndMakeVisible(node.label_component);
    addAndMakeVisible(node.svg_component);
}

SvgWithLabelGrid::~SvgWithLabelGrid()
{
    for (auto c : components)
        delete c;
}

void SvgWithLabelGrid::resized()
{
    const auto labelHeight = ViewUtil::getLabelHeight(node.label, getScale);
    const auto drawableBounds = dynamic_cast<SvgComponent*>(node.svg_component)->getDrawableBounds();
    const auto scale = getScale();
    const auto svgWidth = drawableBounds.getWidth() * scale;

    juce::Grid grid;
    grid.justifyItems = juce::Grid::JustifyItems::center;

    grid.templateRows = { juce::Grid::Px(labelHeight), juce::Grid::Px(ViewUtil::getLabelHeight("", getScale) * 0.2f), juce::Grid::Fr(1) };
    grid.templateColumns = { juce::Grid::Fr(1) };

    grid.items.add(juce::GridItem(node.label_component).withArea(1, 1, 1, 1));
    grid.items.add(juce::GridItem(node.svg_component).withArea(3, 1, 3, 1).withWidth(node.width == "auto" ? juce::GridItem::notAssigned : svgWidth));

    grid.performLayout(getLocalBounds());
}

