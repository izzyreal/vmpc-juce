#include "ViewUtil.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include "GridWrapper.hpp"
#include "FlexBoxWrapper.hpp"
#include "SvgComponent.hpp"
#include "KeyComponent.hpp"
#include "CursorKeys.hpp"
#include "SvgWithLabelGrid.hpp"
#include "LineFlankedLabel.hpp"
#include "RectangleLabel.hpp"
#include "JOrLShape.hpp"
#include "Rectangle.hpp"
#include "Constants.hpp"
#include "NumKey.hpp"
#include "SliderBorder.hpp"
#include "Slider.hpp"
#include "Shadow.hpp"
#include "DataWheel.hpp"
#include "Pot.hpp"
#include "Lcd.hpp"
#include "Led.hpp"
#include "Pad.hpp"
#include "KeyTooltip.hpp"
#include "MpcHardwareMouseListener.hpp"

#include "hardware/ComponentId.hpp"
#include "hardware/Hardware.hpp"
#include "input/keyboard/KeyCodeHelper.hpp"
#include "sequencer/Sequencer.hpp"
#include "sequencer/Transport.hpp"

using namespace vmpc_juce::gui::vector;

float ViewUtil::getLabelHeight(const std::string &text,
                               const std::function<float()> &getScale)
{
    const auto newlineCount = (float)std::count(text.begin(), text.end(), '\n');

    return ((Constants::BASE_FONT_SIZE * (newlineCount + 1)) +
            (Constants::LINE_SIZE * newlineCount)) *
           getScale();
}

static void addShadow(const node &n, const std::function<float()> &getScale,
                      SvgComponent *svgComponent, juce::Component *parent,
                      std::vector<juce::Component *> &components)
{
    if (n.shadow_size == 0.f || svgComponent == nullptr)
    {
        return;
    }

    const auto getShadowPath = [svgComponent]() -> juce::Path
    {
        return svgComponent->getShadowPath();
    };
    const auto getShadowSizeMultiplier = [svgComponent]() -> float
    {
        return svgComponent->getShadowSizeMultiplier();
    };
    const auto getShadowDarknessMultiplier = [svgComponent]() -> float
    {
        return svgComponent->getShadowDarknessMultiplier();
    };

    const auto shadowDarkness =
        n.shadow_darkness > 0.f ? n.shadow_darkness : 0.4f;
    auto shadow = new Shadow(getScale, getShadowPath, getShadowSizeMultiplier,
                             getShadowDarknessMultiplier, n.shadow_size,
                             shadowDarkness, n.is_inner_shadow);
    svgComponent->shadow = shadow;
    components.push_back(shadow);
    parent->addAndMakeVisible(shadow);
}

static juce::Component *createSvgLikeComponent(
    mpc::Mpc &mpc, const node &n, juce::Component *parent,
    const std::function<float()> &getScale)
{
    if (n.node_type == "cursor_keys")
    {
        using ComponentId = mpc::hardware::ComponentId;
        return new CursorKeys(
            {mpc.getHardware()->getButton(ComponentId::CURSOR_LEFT_OR_DIGIT),
             mpc.getHardware()->getButton(ComponentId::CURSOR_UP),
             mpc.getHardware()->getButton(ComponentId::CURSOR_RIGHT_OR_DIGIT),
             mpc.getHardware()->getButton(ComponentId::CURSOR_DOWN)},
            parent, getScale, n.shadow_size);
    }

    if (!n.key_hole_svg.empty() && !n.key_button_svg.empty())
    {
        std::shared_ptr<mpc::hardware::Button> trackedButton;

        if (!n.hardware_label.empty())
        {
            const auto componentIdIt =
                mpc::hardware::componentLabelToId.find(n.hardware_label);

            if (componentIdIt != mpc::hardware::componentLabelToId.end() &&
                mpc::hardware::isButtonId(componentIdIt->second))
            {
                trackedButton = mpc.getHardware()->getButton(componentIdIt->second);
            }
        }

        return new KeyComponent(n.key_hole_svg, n.key_button_svg, trackedButton,
                                parent, n.shadow_size, getScale);
    }

    return new SvgComponent({n.svg}, parent, n.shadow_size, getScale,
                            n.svg_placement);
}

static SvgComponent *getShadowSvgComponent(juce::Component *component)
{
    if (auto svgComponent = dynamic_cast<SvgComponent *>(component))
    {
        return svgComponent;
    }

    return nullptr;
}

void ViewUtil::createComponent(
    mpc::Mpc &mpc, node &n, std::vector<juce::Component *> &components,
    juce::Component *parent, const std::function<float()> &getScale,
    const std::function<juce::Font &()> &getMainFontScaled,
    const std::function<juce::Font &()> &getMpc2000xlFaceplateGlyphsScaled,
    const std::function<juce::Font &()> &getKeyTooltipFontScaled,
    std::vector<juce::MouseListener *> &mouseListeners,
    juce::Component *tooltipOverlay)
{
    juce::Component *tooltipAnchor = nullptr;

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
        createComponents(mpc, n, gridWrapper->components, gridWrapper, getScale,
                         getMainFontScaled, getMpc2000xlFaceplateGlyphsScaled,
                         getKeyTooltipFontScaled, mouseListeners,
                         tooltipOverlay);
        components.push_back(gridWrapper);
        parent->addAndMakeVisible(gridWrapper);
        n.grid_wrapper_component = gridWrapper;
    }
    else if (n.node_type == "flex_box")
    {
        auto flexBoxWrapper = new FlexBoxWrapper(n, getScale);
        createComponents(
            mpc, n, flexBoxWrapper->components, flexBoxWrapper, getScale,
            getMainFontScaled, getMpc2000xlFaceplateGlyphsScaled,
            getKeyTooltipFontScaled, mouseListeners, tooltipOverlay);
        components.push_back(flexBoxWrapper);
        parent->addAndMakeVisible(flexBoxWrapper);
        n.flex_box_wrapper_component = flexBoxWrapper;
    }
    else if (n.node_type == "line_flanked_label")
    {
        auto lineFlankedLabel =
            new LineFlankedLabel(n.label, getScale, getMainFontScaled);
        components.push_back(lineFlankedLabel);
        parent->addAndMakeVisible(lineFlankedLabel);
        n.line_flanked_label_component = lineFlankedLabel;
    }
    else if (n.node_type == "j_shape" || n.node_type == "l_shape")
    {
        const auto jOrLShape =
            new JOrLShape(n.node_type == "j_shape" ? JOrLShape::Shape::J
                                                   : JOrLShape::Shape::L,
                          getScale);
        components.push_back(jOrLShape);
        parent->addAndMakeVisible(jOrLShape);
        n.j_or_l_shape_component = jOrLShape;
    }
    else if (n.node_type == "face_paint_grey_rectangle" ||
             n.node_type == "chassis_rectangle" ||
             n.node_type == "lcd_rectangle")
    {
        const auto rectangle = new Rectangle(
            n.node_type == "chassis_rectangle" ? Constants::chassisColour
            : n.node_type == "lcd_rectangle"   ? Constants::lcdOff
                                             : Constants::greyFacePaintColour);
        components.push_back(rectangle);
        parent->addAndMakeVisible(rectangle);
        n.rectangle_component = rectangle;
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
            if (doingTop)
            {
                topLabel += c;
            }
            else
            {
                bottomLabel += c;
            }
        }

        const auto numKey =
            new NumKey(getScale, topLabel, bottomLabel, n.svg, n.key_hole_svg,
                       n.key_button_svg,
                       !n.hardware_label.empty() &&
                               mpc::hardware::componentLabelToId.count(
                                   n.hardware_label) > 0 &&
                               mpc::hardware::isButtonId(
                                   mpc::hardware::componentLabelToId.at(
                                       n.hardware_label))
                           ? mpc.getHardware()->getButton(
                                 mpc::hardware::componentLabelToId.at(
                                     n.hardware_label))
                           : nullptr,
                       parent, n.shadow_size,
                       getMainFontScaled);
        addShadow(n, getScale, numKey->getSvgComponent(), parent, components);
        components.push_back(numKey);
        parent->addAndMakeVisible(numKey);
        n.num_key_component = numKey;
        tooltipAnchor = numKey;
    }
    else if (n.node_type == "slider_border")
    {
        auto sliderBorder = new SliderBorder(getScale, getMainFontScaled);
        parent->addAndMakeVisible(sliderBorder);
        components.push_back(sliderBorder);
        n.slider_border_component = sliderBorder;
    }
    else if (n.node_type == "slider")
    {
        auto slider = new Slider(mpc, mpc.getHardware()->getSlider(), getScale);
        parent->addAndMakeVisible(slider);
        components.push_back(slider);
        n.slider_component = slider;
    }
    else if (n.node_type == "data_wheel")
    {
        auto dataWheel = new DataWheel(mpc.getHardware()->getDataWheel(),
                                       parent, n.shadow_size, getScale);
        addShadow(n, getScale, dataWheel->backgroundSvg, parent, components);
        n.data_wheel_component = dataWheel;
        parent->addAndMakeVisible(dataWheel);
        components.push_back(dataWheel);
        tooltipAnchor = dataWheel;
    }
    else if (n.node_type == "lcd")
    {
        auto lcd = new Lcd(mpc);
        if (n.magic_multiplier > 0.f)
        {
            lcd->magicMultiplier = n.magic_multiplier;
        }
        n.lcd_component = lcd;
        parent->addAndMakeVisible(lcd);
        components.push_back(lcd);
    }
    else if (n.node_type == "red_led" || n.node_type == "green_led")
    {
        auto mpcLed = mpc.getHardware()->getLed(
            mpc::hardware::componentLabelToId.at(n.hardware_label));

        auto getTransport = [sequencer = mpc.getSequencer()]
        {
            return sequencer->getTransport();
        };

        auto led = new Led(mpcLed, getTransport, mpc.clientEventController,
                           n.node_type == "red_led" ? Led::LedColor::RED
                                                    : Led::LedColor::GREEN,
                           getScale);
        n.led_component = led;
        parent->addAndMakeVisible(led);
        components.push_back(led);
    }
    else if (n.node_type == "pad")
    {
        assert(n.name.length() == 5 || n.name.length() == 6);
        const auto digitsString = n.name.substr(4);
        const auto padNumber = std::stoi(digitsString);
        auto mpcPad = mpc.getHardware()->getPad(padNumber - 1);
        auto pad = new Pad(parent, n.shadow_size, getScale, mpc, mpcPad);
        n.svg_component = pad;
        components.push_back(pad);
        addShadow(n, getScale, pad, parent, components);
        parent->addAndMakeVisible(pad);
        tooltipAnchor = pad;
    }
    else if (!n.svg.empty() && n.label.empty())
    {
        auto svgComponent = createSvgLikeComponent(mpc, n, parent, getScale);
        components.push_back(svgComponent);
        if (n.node_type != "cursor_keys")
        {
            addShadow(n, getScale, getShadowSvgComponent(svgComponent), parent,
                      components);
        }
        parent->addAndMakeVisible(svgComponent);
        n.svg_component = svgComponent;

        if (n.hide_svg)
        {
            svgComponent->setVisible(false);
        }
        tooltipAnchor = svgComponent;
    }
    else if (!n.svg.empty() && !n.label.empty())
    {
        LabelComponent *labelComponent;

        if (n.label_style == "function_key")
        {
            labelComponent = new RectangleLabel(
                getScale, n.label, n.label, Constants::greyFacePaintColour,
                Constants::darkLabelColour, 0.5f, 10.f, getMainFontScaled);
        }
        else
        {
            labelComponent = new SimpleLabel(
                getScale, n.label, Constants::labelColour, getMainFontScaled);
        }

        juce::Component *svgLikeComponent = nullptr;

        if (n.name == "rec_gain")
        {
            svgLikeComponent = new Pot(mpc.getHardware()->getRecPot(),
                                       Pot::PotType::REC_GAIN, parent,
                                       getScale);
        }
        else if (n.name == "main_volume")
        {
            svgLikeComponent = new Pot(mpc.getHardware()->getVolPot(),
                                       Pot::PotType::MAIN_VOLUME, parent,
                                       getScale);
        }
        else
        {
            svgLikeComponent = createSvgLikeComponent(mpc, n, parent, getScale);
        }

        n.svg_component = svgLikeComponent;
        n.label_component = labelComponent;

        if (dynamic_cast<GridWrapper *>(parent))
        {
            auto svgWithLabelGrid = new SvgWithLabelGrid(n, getScale);
            svgWithLabelGrid->components.push_back(labelComponent);
            svgWithLabelGrid->addAndMakeVisible(labelComponent);

            svgWithLabelGrid->components.push_back(svgLikeComponent);

            components.push_back(svgWithLabelGrid);

            if (n.node_type != "cursor_keys")
            {
                addShadow(n, getScale, getShadowSvgComponent(svgLikeComponent),
                          parent, components);
            }

            parent->addAndMakeVisible(svgWithLabelGrid);
            n.svg_with_label_grid_component = svgWithLabelGrid;

            if (n.name != "rec_gain" && n.name != "main_volume")
            {
                tooltipAnchor = svgWithLabelGrid;
            }
        }
        else /* if parent is FlexBoxWrapper */
        {
            components.push_back(svgLikeComponent);
            parent->addAndMakeVisible(svgLikeComponent);

            components.push_back(labelComponent);
            parent->addAndMakeVisible(labelComponent);

            tooltipAnchor = svgLikeComponent;
        }
    }
    else if (!n.label.empty())
    {
        LabelComponent *labelComponent = nullptr;

        const auto fontGetter = n.font == "faceplate-glyphs"
                                    ? getMpc2000xlFaceplateGlyphsScaled
                                    : getMainFontScaled;

        if (n.label_style == "chassis_background")
        {
            labelComponent = new RectangleLabel(
                getScale, n.label, n.label_text_to_calculate_width,
                Constants::chassisColour, Constants::darkLabelColour, 0.f, 2.f,
                fontGetter);
        }
        else if (n.label_style == "rounded")
        {
            labelComponent = new RectangleLabel(
                getScale, n.label, n.label_text_to_calculate_width,
                Constants::darkLabelColour, Constants::chassisColour, 1.5f, 6.f,
                fontGetter);
        }
        else if (n.label_style == "pad_letters")
        {
            labelComponent = new SimpleLabel(
                getScale, n.label, Constants::betweenChassisAndLabelColour,
                fontGetter);
        }
        else if (n.label_style == "cursor_digit")
        {
            labelComponent = new RectangleLabel(
                getScale, n.label, n.label, Constants::greyFacePaintColour,
                Constants::darkLabelColour, 0.5f, 5.f, fontGetter);
        }
        else if (n.label_style == "dark")
        {
            labelComponent = new SimpleLabel(
                getScale, n.label, Constants::darkLabelColour, fontGetter);
        }
        else
        {
            labelComponent = new SimpleLabel(
                getScale, n.label, Constants::labelColour, fontGetter);
        }

        n.label_component = labelComponent;
        components.push_back(labelComponent);
        parent->addAndMakeVisible(labelComponent);
    }

    if (!n.hardware_label.empty())
    {
        std::string label = n.hardware_label;
        const std::string suffix = "-led";

        if (label.size() > suffix.size() &&
            label.compare(label.size() - suffix.size(), suffix.size(),
                          suffix) == 0)
        {
            label.erase(label.size() - suffix.size());
        }

        auto mouseListener = new MpcHardwareMouseListener(mpc, label);
        mouseListeners.push_back(mouseListener);

        for (auto it = components.rbegin(); it != components.rend(); ++it)
        {
            if (dynamic_cast<Shadow *>(*it) != nullptr)
            {
                continue;
            }

            (*it)->addMouseListener(mouseListener, true);
            break;
        }

        if (tooltipAnchor != nullptr)
        {
            using ComponentId = mpc::hardware::ComponentId;
            std::vector<ComponentId> componentIds;
            std::vector<std::pair<float, float>> unscaledOffsetsFromAnchor;

            if (n.node_type == "data_wheel")
            {
                componentIds = {ComponentId::DATA_WHEEL};
                unscaledOffsetsFromAnchor = {{-12.f, 0.f}, {10.f, 0.f}};
            }
            else if (n.hardware_label == "cursor")
            {
                componentIds = {ComponentId::CURSOR_LEFT_OR_DIGIT,
                                ComponentId::CURSOR_RIGHT_OR_DIGIT,
                                ComponentId::CURSOR_UP,
                                ComponentId::CURSOR_DOWN};
                unscaledOffsetsFromAnchor = {
                    {-15.f, 0.f}, {15.f, 0.f}, {0.f, -9.f}, {0.f, 9.f}};
            }
            else
            {
                componentIds = {
                    mpc::hardware::componentLabelToId.at(n.hardware_label)};
                unscaledOffsetsFromAnchor = {{0.f, 0.f}};
            }

            size_t offsetsIndex = 0;

            for (size_t i = 0; i < componentIds.size(); i++)
            {
                auto keycodes = mpc.clientEventController->getKeyboardBindings()
                                    ->lookupComponentKeyCodes(componentIds[i]);

                for (size_t j = 0; j < keycodes.size(); j++)
                {
                    if (offsetsIndex >= unscaledOffsetsFromAnchor.size())
                    {
                        break;
                    }

                    const auto offset =
                        unscaledOffsetsFromAnchor[offsetsIndex++];

                    const auto getTooltipText = [&mpc, keycode = keycodes[j]]
                    {
                        const auto keyboardBindings =
                            mpc.clientEventController->getKeyboardBindings();
                        return mpc::input::keyboard::KeyCodeHelper::
                            guessCharactersPrintedOnKeyUnicode(keycode);
                    };

                    const auto tooltip = new KeyTooltip(
                        getTooltipText, tooltipAnchor, offset,
                        getKeyTooltipFontScaled, getScale, n.hardware_label);
                    components.push_back(tooltip);
                    tooltipOverlay->addChildComponent(tooltip);
                }
            }
        }
    }

    if (auto labelComponent = dynamic_cast<LabelComponent *>(n.label_component);
        labelComponent != nullptr && n.font_scale > 0.f)
    {
        labelComponent->setFontScale(n.font_scale);
    }
}

void ViewUtil::createComponents(
    mpc::Mpc &mpc, node &n, std::vector<juce::Component *> &components,
    juce::Component *parent, const std::function<float()> &getScale,
    const std::function<juce::Font &()> &getMainFontScaled,
    const std::function<juce::Font &()> &getMpc2000xlFaceplateGlyphsScaled,
    const std::function<juce::Font &()> &getKeyTooltipFontScaled,
    std::vector<juce::MouseListener *> &mouseListeners,
    juce::Component *tooltipOverlay)
{
    for (auto &c : n.children)
    {
        createComponent(mpc, c, components, parent, getScale, getMainFontScaled,
                        getMpc2000xlFaceplateGlyphsScaled,
                        getKeyTooltipFontScaled, mouseListeners,
                        tooltipOverlay);
    }
}
