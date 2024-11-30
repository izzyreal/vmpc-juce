#include "SvgWithLabelGrid.hpp"

#include "ViewUtil.hpp"
#include "SvgComponent.hpp"

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
    const auto svgWidth = drawableBounds.getWidth() * getScale();

    juce::Grid grid;
    grid.justifyItems = juce::Grid::JustifyItems::center;

    grid.templateRows = { juce::Grid::Px(labelHeight), juce::Grid::Px(ViewUtil::getLabelHeight("", getScale) * 0.2f), juce::Grid::Fr(1) };
    grid.templateColumns = { juce::Grid::Fr(1) };

    grid.items.add(juce::GridItem(node.label_component).withArea(1, 1, 1, 1));
    grid.items.add(juce::GridItem(node.svg_component).withArea(3, 1, 3, 1).withWidth(svgWidth));

    grid.performLayout(getLocalBounds());
}

