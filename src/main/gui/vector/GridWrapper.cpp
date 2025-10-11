#include "GridWrapper.hpp"

#include "FlexBoxWrapper.hpp"
#include "LabelComponent.hpp"
#include "SvgComponent.hpp"
#include "DataWheel.hpp"

#include <cassert>

using namespace vmpc_juce::gui::vector;

GridWrapper::GridWrapper(struct node &myNodeToUse, const std::function<float()> &getScaleToUse)
    : myNode(myNodeToUse), getScale(getScaleToUse)
{
    setInterceptsMouseClicks(false, true);
}

GridWrapper::~GridWrapper()
{
    for (auto c : components)
        delete c;
}

static void processChildren(
        juce::Grid& parent,
        const std::vector<node>& children,
        const float scale)
{
    for (auto& c : children)
    {
        juce::Component* component = nullptr;
        float width = juce::GridItem::notAssigned;

        if (c.node_type == "grid")
        {
            component = c.grid_wrapper_component;
        }
        else if (c.node_type == "flex_box")
        {
            component = c.flex_box_wrapper_component;
        }
        else if (c.node_type == "line_flanked_label")
        {
            component = c.line_flanked_label_component;
        }
        else if (c.node_type == "j_shape" || c.node_type == "l_shape")
        {
            component = c.j_or_l_shape_component;
        }
        else if (c.node_type == "face_paint_grey_rectangle" || c.node_type == "chassis_rectangle" || c.node_type == "lcd_rectangle")
        {
            component = c.rectangle_component;
        }
        else if (c.node_type == "slider_border")
        {
            component = c.slider_border_component;
        }
        else if (c.node_type == "slider")
        {
            component = c.slider_component;
        }
        else if (c.node_type == "data_wheel")
        {
            component = c.data_wheel_component;
            const auto drawableBounds = dynamic_cast<DataWheel*>(c.data_wheel_component)->getDrawableBounds();
            width = drawableBounds.getWidth() * scale;
        }
        else if (c.node_type == "lcd")
        {
            component = c.lcd_component;
        }
        else if (c.svg_with_label_grid_component != nullptr)
        {
            component = c.svg_with_label_grid_component;
        }
        else if (c.svg_component != nullptr)
        {
           // The case where there's both an SVG, as well as a label, should be handled by svg_with_label_grid_component.
           // Hence we make sure there's no label Component associated with this myNode.
            assert(c.label_component == nullptr);
            component = c.svg_component;

            if (c.width == "auto")
            {
                width = juce::GridItem::notAssigned;
            }
            else
            {
                const auto drawableBounds = dynamic_cast<SvgComponent*>(component)->getDrawableBounds();

                if (!drawableBounds.isEmpty())
                {
                    width = drawableBounds.getWidth() * scale;
                }
            }
        }
        else if (c.node_type == "num_key")
        {
            component = c.num_key_component;
        }
        else if (c.node_type == "red_led" || c.node_type == "green_led")
        {
            component = c.led_component;
            const auto drawableBounds = dynamic_cast<SvgComponent*>(c.led_component)->getDrawableBounds();
            width = drawableBounds.getWidth() * scale;
        }
        else if (c.label_component != nullptr)
        {
            width = dynamic_cast<LabelComponent*>(c.label_component)->getRequiredWidth();
            component = c.label_component;
        }

        if (component != nullptr)
        {
            const auto margin = c.margins.empty() ?
                juce::GridItem::Margin(c.margin * scale) :
                juce::GridItem::Margin(c.margins[0] * scale, c.margins[1] * scale,
                        c.margins[2] * scale, c.margins[3] * scale);

            const auto area = c.area;

            const auto item = juce::GridItem(component)
                .withMargin(margin)
                .withWidth(width)
                .withArea(area[0], area[1], area[2], area[3]);

            parent.items.add(item);
        }
    }
}

void GridWrapper::resized()
{
    juce::Grid grid;
    grid.justifyItems = juce::Grid::JustifyItems::center;

    if (myNode.justify_items == "start")
    {
        grid.justifyItems = juce::Grid::JustifyItems::start;
    }
    else if (myNode.justify_items == "end")
    {
        grid.justifyItems = juce::Grid::JustifyItems::end;
    }

    juce::Array<juce::Grid::TrackInfo> rowTrackInfos;
    juce::Array<juce::Grid::TrackInfo> columnTrackInfos;

    for (auto& f : myNode.row_fractions)
        rowTrackInfos.add(juce::Grid::TrackInfo(juce::Grid::Fr(f)));

    for (auto& f : myNode.column_fractions)
        columnTrackInfos.add(juce::Grid::TrackInfo(juce::Grid::Fr(f)));

    grid.templateRows = rowTrackInfos;
    grid.templateColumns = columnTrackInfos;

    processChildren(grid, myNode.children, getScale());

    grid.performLayout(getLocalBounds());
}

