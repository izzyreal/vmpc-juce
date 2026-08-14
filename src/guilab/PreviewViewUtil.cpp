#include "PreviewViewUtil.hpp"

#include "PreviewLcd.hpp"

#include "gui/vector/AdditionalShadowComponentsProvider.hpp"
#include "gui/vector/Constants.hpp"
#include "gui/vector/CursorKeys.hpp"
#include "gui/vector/DataWheel.hpp"
#include "gui/vector/FlexBoxWrapper.hpp"
#include "gui/vector/GridWrapper.hpp"
#include "gui/vector/JOrLShape.hpp"
#include "gui/vector/KeyComponent.hpp"
#include "gui/vector/LabelComponent.hpp"
#include "gui/vector/Led.hpp"
#include "gui/vector/LineFlankedLabel.hpp"
#include "gui/vector/NumKey.hpp"
#include "gui/vector/Pot.hpp"
#include "gui/vector/Rectangle.hpp"
#include "gui/vector/RectangleLabel.hpp"
#include "gui/vector/Shadow.hpp"
#include "gui/vector/SimpleLabel.hpp"
#include "gui/vector/Slider.hpp"
#include "gui/vector/SliderBorder.hpp"
#include "gui/vector/SvgComponent.hpp"
#include "gui/vector/SvgWithLabelGrid.hpp"

#include "hardware/Component.hpp"

#include <array>
#include <memory>

using namespace vmpc_juce::guilab;
using namespace vmpc_juce::gui::vector;

namespace
{
    void addShadow(const node &n, const std::function<float()> &getScale,
                   SvgComponent *svgComponent, juce::Component *parent,
                   std::vector<juce::Component *> &components)
    {
        if (n.shadow_size == 0.f || svgComponent == nullptr)
        {
            return;
        }

        auto *shadow = new Shadow(
            getScale,
            [svgComponent]
            {
                return svgComponent->getShadowPath();
            },
            [svgComponent]
            {
                return svgComponent->getShadowSizeMultiplier();
            },
            [svgComponent]
            {
                return svgComponent->getShadowDarknessMultiplier();
            },
            n.shadow_size, n.shadow_darkness > 0.f ? n.shadow_darkness : 0.4f,
            n.is_inner_shadow);

        svgComponent->shadow = shadow;
        components.push_back(shadow);
        parent->addAndMakeVisible(shadow);
    }

    void addShadows(const node &n, const std::function<float()> &getScale,
                    juce::Component *component, juce::Component *parent,
                    std::vector<juce::Component *> &components)
    {
        if (auto *provider =
                dynamic_cast<AdditionalShadowComponentsProvider *>(component))
        {
            for (auto *shadowComponent :
                 provider->getAdditionalShadowComponents())
            {
                addShadow(n, getScale, shadowComponent, parent, components);
            }
            return;
        }

        addShadow(n, getScale, dynamic_cast<SvgComponent *>(component), parent,
                  components);
    }

    bool hasSvgLikeVisual(const node &n)
    {
        return !n.svg.empty() ||
               (!n.key_hole_svg.empty() && !n.key_button_svg.empty());
    }

    juce::Component *
    createSvgLikeComponent(const node &n, juce::Component *parent,
                           const std::function<float()> &getScale)
    {
        if (n.name == "rec_gain" || n.name == "main_volume")
        {
            const auto type =
                n.name == "rec_gain" ? Pot::REC_GAIN : Pot::MAIN_VOLUME;
            const auto id = n.name == "rec_gain"
                                ? mpc::hardware::REC_GAIN_POT
                                : mpc::hardware::MAIN_VOLUME_POT;
            auto model = std::make_shared<mpc::hardware::Pot>(id);
            model->setValue(0.5f);
            auto *pot = new Pot(model, type, parent, getScale);
            pot->sharedTimerCallback();
            return pot;
        }

        if (n.node_type == "cursor_keys")
        {
            return new CursorKeys(
                std::array<std::shared_ptr<mpc::hardware::Button>, 4>{}, parent,
                getScale, n.shadow_size);
        }

        if (!n.key_hole_svg.empty() && !n.key_button_svg.empty())
        {
            return new KeyComponent(n.key_hole_svg, n.key_button_svg, nullptr,
                                    parent, n.shadow_size, getScale);
        }

        return new SvgComponent({n.svg}, parent, n.shadow_size, getScale,
                                n.svg_placement);
    }

    void resetComponentPointers(node &n)
    {
        n.svg_component = nullptr;
        n.label_component = nullptr;
        n.grid_wrapper_component = nullptr;
        n.flex_box_wrapper_component = nullptr;
        n.svg_with_label_grid_component = nullptr;
        n.line_flanked_label_component = nullptr;
        n.j_or_l_shape_component = nullptr;
        n.rectangle_component = nullptr;
        n.num_key_component = nullptr;
        n.slider_border_component = nullptr;
        n.slider_component = nullptr;
        n.data_wheel_component = nullptr;
        n.lcd_component = nullptr;
        n.led_component = nullptr;
    }
} // namespace

void PreviewViewUtil::createComponent(
    node &n, std::vector<juce::Component *> &components,
    juce::Component *parent, const std::function<float()> &getScale,
    const std::function<juce::Font &()> &getMainFontScaled,
    const std::function<juce::Font &()> &getMpc2000xlFaceplateGlyphsScaled)
{
    resetComponentPointers(n);

    if (n.node_type == "grid")
    {
        auto *wrapper = new GridWrapper(n, getScale);
        createComponents(n, wrapper->components, wrapper, getScale,
                         getMainFontScaled, getMpc2000xlFaceplateGlyphsScaled);
        components.push_back(wrapper);
        parent->addAndMakeVisible(wrapper);
        n.grid_wrapper_component = wrapper;
    }
    else if (n.node_type == "flex_box")
    {
        auto *wrapper = new FlexBoxWrapper(n, getScale);
        createComponents(n, wrapper->components, wrapper, getScale,
                         getMainFontScaled, getMpc2000xlFaceplateGlyphsScaled);
        components.push_back(wrapper);
        parent->addAndMakeVisible(wrapper);
        n.flex_box_wrapper_component = wrapper;
    }
    else if (n.node_type == "line_flanked_label")
    {
        auto *label =
            new LineFlankedLabel(n.label, getScale, getMainFontScaled);
        components.push_back(label);
        parent->addAndMakeVisible(label);
        n.line_flanked_label_component = label;
    }
    else if (n.node_type == "j_shape" || n.node_type == "l_shape")
    {
        auto *shape =
            new JOrLShape(n.node_type == "j_shape" ? JOrLShape::Shape::J
                                                   : JOrLShape::Shape::L,
                          getScale);
        components.push_back(shape);
        parent->addAndMakeVisible(shape);
        n.j_or_l_shape_component = shape;
    }
    else if (n.node_type == "face_paint_grey_rectangle" ||
             n.node_type == "chassis_rectangle" ||
             n.node_type == "lcd_rectangle")
    {
        auto *rectangle = new Rectangle(
            n.node_type == "chassis_rectangle" ? Constants::chassisColour
            : n.node_type == "lcd_rectangle"   ? Constants::lcdOff
                                             : Constants::greyFacePaintColour);
        components.push_back(rectangle);
        parent->addAndMakeVisible(rectangle);
        n.rectangle_component = rectangle;
    }
    else if (n.node_type == "num_key")
    {
        const auto split = n.label.find('\n');
        const auto top = n.label.substr(0, split);
        const auto bottom = split == std::string::npos
                                ? std::string{}
                                : n.label.substr(split + 1);
        auto *numKey =
            new NumKey(getScale, top, bottom, n.key_hole_svg, n.key_button_svg,
                       nullptr, parent, n.shadow_size, getMainFontScaled);
        addShadow(n, getScale, numKey->getSvgComponent(), parent, components);
        components.push_back(numKey);
        parent->addAndMakeVisible(numKey);
        n.num_key_component = numKey;
    }
    else if (n.node_type == "slider_border")
    {
        auto *border = new SliderBorder(getScale, getMainFontScaled);
        components.push_back(border);
        parent->addAndMakeVisible(border);
        n.slider_border_component = border;
    }
    else if (n.node_type == "slider")
    {
        auto model = std::make_shared<mpc::hardware::Slider>();
        model->setValue(63.5f);
        auto *slider = new Slider(model, parent, n.shadow_size, getScale);
        addShadows(n, getScale, slider, parent, components);
        parent->addAndMakeVisible(slider);
        components.push_back(slider);
        n.slider_component = slider;
    }
    else if (n.node_type == "data_wheel")
    {
        auto model = std::make_shared<mpc::hardware::DataWheel>();
        auto *wheel = new DataWheel(model, parent, n.shadow_size, getScale);
        addShadow(n, getScale, wheel->backgroundSvg, parent, components);
        components.push_back(wheel);
        parent->addAndMakeVisible(wheel);
        n.data_wheel_component = wheel;
    }
    else if (n.node_type == "lcd")
    {
        auto *lcd = new PreviewLcd();
        if (n.magic_multiplier > 0.f)
        {
            lcd->magicMultiplier = n.magic_multiplier;
        }
        components.push_back(lcd);
        parent->addAndMakeVisible(lcd);
        n.lcd_component = lcd;
    }
    else if (n.node_type == "red_led" || n.node_type == "green_led")
    {
        auto *led = new SvgComponent({"led_off.svg"}, nullptr, 0.f, getScale);
        components.push_back(led);
        parent->addAndMakeVisible(led);
        n.led_component = led;
    }
    else if (n.node_type == "pad")
    {
        auto *pad =
            new SvgComponent({"pad.svg"}, parent, n.shadow_size, getScale);
        components.push_back(pad);
        addShadow(n, getScale, pad, parent, components);
        parent->addAndMakeVisible(pad);
        n.svg_component = pad;
    }
    else if (hasSvgLikeVisual(n) && n.label.empty())
    {
        auto *component = createSvgLikeComponent(n, parent, getScale);
        components.push_back(component);
        addShadows(n, getScale, component, parent, components);
        parent->addAndMakeVisible(component);
        n.svg_component = component;
        component->setVisible(!n.hide_svg);
    }
    else if (hasSvgLikeVisual(n) && !n.label.empty())
    {
        LabelComponent *label = nullptr;
        if (n.label_style == "function_key")
        {
            label = new RectangleLabel(
                getScale, n.label, n.label, Constants::greyFacePaintColour,
                Constants::darkLabelColour, 0.5f, 10.f, getMainFontScaled);
        }
        else
        {
            label = new SimpleLabel(getScale, n.label, Constants::labelColour,
                                    getMainFontScaled);
        }

        auto *visual = createSvgLikeComponent(n, parent, getScale);

        n.svg_component = visual;
        n.label_component = label;

        if (dynamic_cast<GridWrapper *>(parent) != nullptr)
        {
            auto *combined = new SvgWithLabelGrid(n, getScale);
            combined->components.push_back(label);
            combined->components.push_back(visual);
            components.push_back(combined);
            addShadows(n, getScale, visual, parent, components);
            parent->addAndMakeVisible(combined);
            n.svg_with_label_grid_component = combined;
        }
        else
        {
            components.push_back(visual);
            components.push_back(label);
            parent->addAndMakeVisible(visual);
            parent->addAndMakeVisible(label);
        }
    }
    else if (!n.label.empty())
    {
        const auto fontGetter = n.font == "faceplate-glyphs"
                                    ? getMpc2000xlFaceplateGlyphsScaled
                                    : getMainFontScaled;
        LabelComponent *label = nullptr;

        if (n.label_style == "chassis_background")
        {
            label = new RectangleLabel(
                getScale, n.label, n.label_text_to_calculate_width,
                Constants::chassisColour, Constants::darkLabelColour, 0.f, 2.f,
                fontGetter);
        }
        else if (n.label_style == "rounded")
        {
            label = new RectangleLabel(
                getScale, n.label, n.label_text_to_calculate_width,
                Constants::darkLabelColour, Constants::chassisColour, 1.5f, 6.f,
                fontGetter);
        }
        else if (n.label_style == "pad_letters")
        {
            label = new SimpleLabel(getScale, n.label,
                                    Constants::betweenChassisAndLabelColour,
                                    fontGetter);
        }
        else if (n.label_style == "cursor_digit")
        {
            label = new RectangleLabel(
                getScale, n.label, n.label, Constants::greyFacePaintColour,
                Constants::darkLabelColour, 0.5f, 5.f, fontGetter);
        }
        else if (n.label_style == "dark")
        {
            label = new SimpleLabel(getScale, n.label,
                                    Constants::darkLabelColour, fontGetter);
        }
        else
        {
            label = new SimpleLabel(getScale, n.label, Constants::labelColour,
                                    fontGetter);
        }

        components.push_back(label);
        parent->addAndMakeVisible(label);
        n.label_component = label;
    }

    if (auto *label = dynamic_cast<LabelComponent *>(n.label_component);
        label != nullptr && n.font_scale > 0.f)
    {
        label->setFontScale(n.font_scale);
    }
}

void PreviewViewUtil::createComponents(
    node &n, std::vector<juce::Component *> &components,
    juce::Component *parent, const std::function<float()> &getScale,
    const std::function<juce::Font &()> &getMainFontScaled,
    const std::function<juce::Font &()> &getMpc2000xlFaceplateGlyphsScaled)
{
    for (auto &child : n.children)
    {
        createComponent(child, components, parent, getScale, getMainFontScaled,
                        getMpc2000xlFaceplateGlyphsScaled);
    }
}
