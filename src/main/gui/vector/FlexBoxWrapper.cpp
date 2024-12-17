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
#include "FlexBoxWrapper.hpp"

#include "GridWrapper.hpp"
#include "SvgComponent.hpp"
#include "ViewUtil.hpp"
#include "LabelComponent.hpp"
#include "Constants.hpp"

#include <cassert>

using namespace vmpc_juce::gui::vector;

FlexBoxWrapper::FlexBoxWrapper(struct node &nodeToUse, const std::function<float()>& getScaleToUse)
    : node(nodeToUse), getScale(getScaleToUse)
{
    printf("FlexBoxWrapper for %s\n", node.name.c_str());
}

FlexBoxWrapper::~FlexBoxWrapper()
{
    for (auto c : components)
        delete c;
}

static void processSvgWithLabel(
        juce::FlexBox& parent,
        std::vector<std::unique_ptr<juce::FlexBox>>& flexBoxes,
        const float minWidth,
        const float minHeight,
        const float flexGrow,
        const std::string& labelText,
        juce::Component* labelComponent,
        juce::Component* svgComponent,
        const std::function<float()>& getScale)
{
    auto childFlexBox = std::make_unique<juce::FlexBox>();
    childFlexBox->flexDirection = juce::FlexBox::Direction::column;
    const auto min_width_label = dynamic_cast<LabelComponent*>(labelComponent)->getRequiredWidth();

    const auto childFlexBoxMinWidth = std::max<float>((float) min_width_label, minWidth);

    const auto labelHeight = ViewUtil::getLabelHeight(labelText, getScale);

    parent.items.add(juce::FlexItem(*childFlexBox).withMinWidth(childFlexBoxMinWidth).withFlex(flexGrow));
    flexBoxes.push_back(std::move(childFlexBox));

    auto label_item = juce::FlexItem(*labelComponent)
        .withMinWidth(childFlexBoxMinWidth)
        .withMinHeight(labelHeight)
        .withMargin(juce::FlexItem::Margin(0.f, 0.f, Constants::BASE_FONT_SIZE * 0.5f * getScale(), 0.f));

    flexBoxes.back()->items.add(label_item);
    flexBoxes.back()->items.add(juce::FlexItem(*svgComponent).withMinWidth(minWidth).withMinHeight(minHeight));
}

static void processChildren(
        juce::FlexBox& parent,
        const std::vector<node>& children,
        std::function<float()> getScale,
        std::vector<std::unique_ptr<juce::FlexBox>>& flexBoxes)
{
    for (auto& c : children)
    {
        const float flexGrow = c.flex_grow > 0 ? c.flex_grow : 1.f;

        if (c.node_type == "spacer")
        {
            parent.items.add(juce::FlexItem().withMinWidth(1.f).withFlex(flexGrow));
            continue;
        }

        if (c.node_type == "grid")
        {
            parent.items.add(juce::FlexItem(*c.grid_wrapper_component));
            continue;
        }
        else if (c.node_type == "flex_box")
        {
            parent.items.add(juce::FlexItem(*c.flex_box_wrapper_component).withFlex(flexGrow));
            continue;
        }
        else if (c.node_type == "line_flanked_label")
        {
            parent.items.add(juce::FlexItem(*c.line_flanked_label_component).withFlex(flexGrow));
            continue;
        }
        else if (c.node_type == "face_paint_grey_rectangle" || c.node_type == "chassis_rectangle")
        {
            parent.items.add(juce::FlexItem(*c.rectangle_component).withFlex(flexGrow));
            continue;
        }

        if (c.svg_component == nullptr && c.label_component == nullptr)
        {
            continue;
        }

        if (c.svg_component != nullptr)
        {

            const auto drawable_bounds = dynamic_cast<SvgComponent*>(c.svg_component)->getDrawableBounds();
            const auto minWidth = drawable_bounds.getWidth() * getScale();
            const auto minHeight = drawable_bounds.getHeight() * getScale();

            if (c.label_component == nullptr)
            {
                parent.items.add(juce::FlexItem(*c.svg_component).withMinWidth(minWidth).withMinHeight(minHeight).withFlex(flexGrow));
                continue;
            }

            if (c.label_component != nullptr && c.svg_component != nullptr)
            {
                processSvgWithLabel(parent, flexBoxes, minWidth, minHeight, flexGrow, c.label, c.label_component, c.svg_component, getScale);
                continue;
            }
        }

        if (c.label_component != nullptr)
        {
            parent.items.add(juce::FlexItem(*c.label_component).withFlex(flexGrow));
        }
    }
}

void FlexBoxWrapper::resized()
{
    printf("FlexBoxWrapper for %s resized to %i, %i\n", node.name.c_str(), getWidth(), getHeight());

    std::vector<std::unique_ptr<juce::FlexBox>> flexBoxes;

    juce::FlexBox flexBox;
    flexBox.justifyContent = juce::FlexBox::JustifyContent::center;

    if (node.align_items == "flex_end")
    {
        flexBox.alignItems = juce::FlexBox::AlignItems::flexEnd;
    }
    else if (node.align_items == "flex_start")
    {
        flexBox.alignItems = juce::FlexBox::AlignItems::flexStart;
    }

    if (node.direction == "column")
    {
        flexBox.flexDirection = juce::FlexBox::Direction::column;
    }
    else if (node.direction == "row")
    {
        flexBox.flexDirection = juce::FlexBox::Direction::row;
    }

    processChildren(flexBox, node.children, getScale, flexBoxes);

    flexBox.performLayout(getLocalBounds());
}

