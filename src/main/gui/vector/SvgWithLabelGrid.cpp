#include "SvgWithLabelGrid.hpp"

#include "ViewUtil.hpp"
#include "SvgComponent.hpp"

using namespace vmpc_juce::gui::vector;

SvgWithLabelGrid::SvgWithLabelGrid(const struct node &myNodeToUse, const std::function<float()>& getScaleToUse)
    : myNode(myNodeToUse), getScale(getScaleToUse)
{
    addAndMakeVisible(myNode.label_component);
    addAndMakeVisible(myNode.svg_component);
}

SvgWithLabelGrid::~SvgWithLabelGrid()
{
    for (auto c : components)
        delete c;
}

void SvgWithLabelGrid::resized()
{
    const auto labelHeight = ViewUtil::getLabelHeight(myNode.label, getScale);
    const auto drawableBounds = dynamic_cast<SvgComponent*>(myNode.svg_component)->getDrawableBounds();
    const auto scale = getScale();
    const auto svgWidth = drawableBounds.getWidth() * scale;

    juce::Grid grid;
    grid.justifyItems = juce::Grid::JustifyItems::center;

    grid.templateRows = { juce::Grid::Px(labelHeight), juce::Grid::Px(ViewUtil::getLabelHeight("", getScale) * 0.2f), juce::Grid::Fr(1) };
    grid.templateColumns = { juce::Grid::Fr(1) };

    grid.items.add(juce::GridItem(myNode.label_component).withArea(1, 1, 1, 1));
    grid.items.add(juce::GridItem(myNode.svg_component).withArea(3, 1, 3, 1).withWidth(myNode.width == "auto" ? juce::GridItem::notAssigned : svgWidth));

    grid.performLayout(getLocalBounds());
}

