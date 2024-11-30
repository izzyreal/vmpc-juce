#include "ViewUtil.hpp"

#include "GridWrapper.hpp"
#include "FlexBoxWrapper.hpp"
#include "SvgComponent.hpp"
#include "SvgWithLabelGrid.hpp"
#include "LineFlankedLabel.hpp"
#include "RectangleLabel.hpp"
#include "JOrLShape.hpp"
#include "Rectangle.hpp"
#include "Constants.hpp"
#include "NumKey.hpp"
#include "Slider.hpp"
#include "Shadow.hpp"
#include "DataWheel.hpp"
#include "Knob.hpp"
#include "Lcd.hpp"
#include "Led.hpp"

float ViewUtil::getLabelHeight(const std::string& text, const std::function<float()>& getScale)
{
    const auto newlineCount = (float) std::count(text.begin(), text.end(), '\n');

    return ((Constants::BASE_FONT_SIZE * (newlineCount + 1)) + (Constants::LINE_SIZE * newlineCount)) * getScale();
}

static void addShadow(const node &n, const std::function<float()> &getScale, SvgComponent *svgComponent, juce::Component *parent, std::vector<juce::Component*> &components)
{
    if (n.shadow_size == 0.f)
    {
        return;
    }

    const auto getShadowPath = [svgComponent] () -> juce::Path {
        return svgComponent->getShadowPath();
    };

    const auto shadowDarkness = n.shadow_darkness > 0.f ? n.shadow_darkness : 0.4f;
    auto shadow = new Shadow(getScale, getShadowPath, n.shadow_size, shadowDarkness, n.is_inner_shadow);
    svgComponent->shadow = shadow;
    components.emplace_back(shadow);
    parent->addAndMakeVisible(shadow);

}

void ViewUtil::createComponent(
        mpc::Mpc &mpc,
        node &n,
        std::vector<juce::Component*> &components,
        juce::Component* parent,
        const std::function<float()> &getScale,
        const std::function<juce::Font&()> &getNimbusSansScaled)
{
    n.svg_component = nullptr;
    n.svg_with_label_grid_component = nullptr;
    n.label_component = nullptr;
    n.flex_box_wrapper_component = nullptr;
    n.grid_wrapper_component = nullptr;
    n.line_flanked_label_component = nullptr;
    n.j_or_l_shape_component = nullptr;
    n.rectangle_component = nullptr;
    n.num_key_component = nullptr;
    n.slider_component = nullptr;
    n.data_wheel_component = nullptr;
    n.lcd_component = nullptr;

    if (n.node_type == "grid")
    {
        auto gridWrapper = new GridWrapper(n, getScale);
        createComponents(mpc, n, gridWrapper->components, gridWrapper, getScale, getNimbusSansScaled);
        components.emplace_back(gridWrapper);
        parent->addAndMakeVisible(components.back());
        n.grid_wrapper_component = components.back();
        return;
    }
    else if (n.node_type == "flex_box")
    {
        auto flexBoxWrapper = new FlexBoxWrapper(n, getScale);
        createComponents(mpc, n, flexBoxWrapper->components, flexBoxWrapper, getScale, getNimbusSansScaled);
        components.emplace_back(flexBoxWrapper);
        parent->addAndMakeVisible(components.back());
        n.flex_box_wrapper_component = components.back();
        return;
    }
    else if (n.node_type == "line_flanked_label")
    {
        components.emplace_back(new LineFlankedLabel(n.label, getScale, getNimbusSansScaled));
        parent->addAndMakeVisible(components.back());
        n.line_flanked_label_component = components.back();
        return;
    }
    else if (n.node_type == "j_shape" || n.node_type == "l_shape")
    {
        const auto jOrLShape = new JOrLShape(n.node_type == "j_shape" ? JOrLShape::Shape::J : JOrLShape::Shape::L, getScale);
        components.emplace_back(jOrLShape);
        parent->addAndMakeVisible(components.back());
        n.j_or_l_shape_component = components.back();
        return;
    }
    else if (n.node_type == "face_paint_grey_rectangle" || n.node_type == "chassis_rectangle" || n.node_type == "lcd_rectangle")
    {
        const auto rectangle = new Rectangle(n.node_type == "chassis_rectangle" ? Constants::chassisColour : n.node_type == "lcd_rectangle" ? Constants::lcdOff : Constants::greyFacePaintColour);
        components.emplace_back(rectangle);
        parent->addAndMakeVisible(components.back());
        n.rectangle_component = rectangle;
        return;
    }
    else if (n.node_type == "num_key")
    {
        std::string topLabel;
        std::string bottomLabel;

        bool doingTop = true;

        for (auto c : n.label)
        {
            if (c == '\n' && !doingTop)
            {
                break;
            }
            if (c == '\n' && doingTop)
            {
                doingTop = false;
                continue;
            }
            if (doingTop) topLabel += c;
            else bottomLabel += c;
        }

        const auto numKey = new NumKey(getScale, topLabel, bottomLabel, n.svg, parent, n.shadow_size, getNimbusSansScaled);
        addShadow(n, getScale, numKey->getSvgComponent(), parent, components);
        components.push_back(numKey);
        parent->addAndMakeVisible(numKey);
        n.num_key_component = numKey;
        return;
    }
    else if (n.node_type == "slider")
    {
        auto slider = new Slider(parent, getScale, n.shadow_size, getNimbusSansScaled);
        n.slider_component = slider;
        addShadow(n, getScale, slider->sliderCapSvg, parent, components);
        parent->addAndMakeVisible(n.slider_component);
        components.push_back(n.slider_component);
        return;
    }
    else if (n.node_type == "data_wheel")
    {
        auto dataWheel = new DataWheel(parent, n.shadow_size, getScale);
        addShadow(n, getScale, dataWheel->backgroundSvg, parent, components);
        n.data_wheel_component = dataWheel;
        parent->addAndMakeVisible(dataWheel);
        components.push_back(dataWheel);
        return;
    }
    else if (n.node_type == "lcd")
    {
        auto lcd = new Lcd(mpc);
        if (n.magic_multiplier > 0.f) lcd->magicMultiplier = n.magic_multiplier;
        n.lcd_component = lcd;
        parent->addAndMakeVisible(lcd);
        components.push_back(lcd);
        return;
    }
    else if (n.node_type == "red_led" || n.node_type == "green_led")
    {
        auto led = new Led(n.node_type == "red_led" ? Led::LedColor::RED : Led::LedColor::GREEN, getScale);
        n.led_component = led;
        parent->addAndMakeVisible(led);
        components.push_back(led);
        return;
    }

    if (!n.svg.empty() && n.label.empty())
    {
        auto svgComponent = new SvgComponent(n.svg, parent, n.shadow_size, getScale);

        components.emplace_back(svgComponent);

        addShadow(n, getScale, svgComponent, parent, components);

        parent->addAndMakeVisible(svgComponent);
        n.svg_component = svgComponent;
        if (n.hide_svg) svgComponent->setVisible(false);
        return;
    }

    if (!n.svg.empty() && !n.label.empty())
    {
        LabelComponent* labelComponent;

        if (n.label_style == "function_key")
        {
            labelComponent = new RectangleLabel(getScale, n.label, n.label, Constants::greyFacePaintColour, Constants::darkLabelColour, 0.5f, 10.f, getNimbusSansScaled);
        }
        else
        {
            labelComponent = new SimpleLabel(getScale, n.label, Constants::labelColour, getNimbusSansScaled);
        }

        SvgComponent *svgComponent;

        if (n.name == "rec_gain")
        {
            svgComponent = new Knob(Knob::KnobType::REC_GAIN, parent, getScale);
        }
        else if (n.name == "main_volume")
        {
            svgComponent = new Knob(Knob::KnobType::MAIN_VOLUME, parent, getScale);
        }
        else
        {
            svgComponent = new SvgComponent(n.svg, parent, n.shadow_size, getScale);
        }

        n.svg_component = svgComponent;
        n.label_component = labelComponent;

        if (dynamic_cast<GridWrapper*>(parent))
        {
            auto svgWithLabelGrid = new SvgWithLabelGrid(n, getScale);
            svgWithLabelGrid->components.push_back(labelComponent);
            svgWithLabelGrid->addAndMakeVisible(labelComponent);

            svgWithLabelGrid->components.push_back(svgComponent);

            components.push_back(svgWithLabelGrid);
            
            addShadow(n, getScale, svgComponent, parent, components);

            parent->addAndMakeVisible(svgWithLabelGrid);
            n.svg_with_label_grid_component = svgWithLabelGrid;
        }
        else /* if parent is FlexBoxWrapper */
        {
            components.push_back(svgComponent);
            parent->addAndMakeVisible(svgComponent);

            components.push_back(labelComponent);
            parent->addAndMakeVisible(labelComponent);
        }
       

        return;
    }

    if (!n.label.empty())
    {
        LabelComponent* labelComponent = nullptr;

        if (n.label_style == "chassis_background")
        {
            labelComponent = new RectangleLabel(getScale, n.label, n.label_text_to_calculate_width, Constants::chassisColour, Constants::darkLabelColour, 0.f, 2.f, getNimbusSansScaled);
        }
        else if (n.label_style == "rounded")
        {
            labelComponent = new RectangleLabel(getScale, n.label, n.label_text_to_calculate_width, Constants::darkLabelColour, Constants::chassisColour, 1.5f, 6.f, getNimbusSansScaled);
        }
        else if (n.label_style == "pad_letters")
        {
            labelComponent = new SimpleLabel(getScale, n.label, Constants::betweenChassisAndLabelColour, getNimbusSansScaled); 
        }
        else if (n.label_style == "cursor_digit")
        {
            labelComponent = new RectangleLabel(getScale, n.label, n.label, Constants::greyFacePaintColour, Constants::darkLabelColour, 0.5f, 10.f, getNimbusSansScaled);
        }
        else if (n.label_style == "dark")
        {
            labelComponent = new SimpleLabel(getScale, n.label, Constants::darkLabelColour, getNimbusSansScaled); 
        }
        else
        {
            labelComponent = new SimpleLabel(getScale, n.label, Constants::labelColour, getNimbusSansScaled);
        }

        n.label_component = labelComponent;
        components.push_back(labelComponent);
        parent->addAndMakeVisible(labelComponent);
        return;
    }
}

void ViewUtil::createComponents(
        mpc::Mpc &mpc,
        node &n,
        std::vector<juce::Component*> &components,
        juce::Component *parent,
        const std::function<float()> &getScale,
        const std::function<juce::Font&()> &getNimbusSansScaled)
{
    for (auto& c : n.children)
    {
        createComponent(mpc, c, components, parent, getScale, getNimbusSansScaled);
    }
}

