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
#include "Pad.hpp"
#include "Tooltip.hpp"
#include "TooltipOverlay.hpp"

#include "gui/MouseWheelControllable.hpp"

#include "hardware/Hardware.hpp"
#include "hardware/Button.hpp"
#include "hardware/HwPad.hpp"
#include "hardware/DataWheel.hpp"
#include "hardware/Pot.hpp"
#include "hardware/HwSlider.hpp"
#include "controls/KbMapping.hpp"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_gui_basics/juce_gui_basics.h"


using namespace vmpc_juce::gui::vector;

    template<class ComponentClass>
static ComponentClass* getChildComponentOfClass(juce::Component *parent)
{
    for (int i = 0; i < parent->getNumChildComponents(); ++i)
    {
        auto* childComp = parent->getChildComponent(i);

        if (auto c = dynamic_cast<ComponentClass*> (childComp))
            return c;

        if (auto c = getChildComponentOfClass<ComponentClass> (childComp))
            return c;
    }

    return nullptr;
}

class MpcHardwareMouseListener : public juce::MouseListener {
    public:
        MpcHardwareMouseListener(mpc::Mpc &mpcToUse, const std::string labelToUse) : label(labelToUse), mpc(mpcToUse) {}

        void mouseMove(const juce::MouseEvent &e) override
        {
            const auto currentModifiers = juce::ModifierKeys::getCurrentModifiers();

            if (!currentModifiers.isAnyModifierKeyDown())
            {
                return;
            }

            setTooltipVisibility(e.eventComponent, true);
        }

        void mouseExit(const juce::MouseEvent &e) override
        {
            setTooltipVisibility(e.eventComponent, false);
        }

        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
        {
            setTooltipVisibility(event.eventComponent, false);

            if (label == "data-wheel")
            {
                auto dw = mpc.getHardware()->getDataWheel();
                mouseWheelControllable.processWheelEvent(wheel, [&dw](int increment) { dw->turn(increment, false); });
            }
            else if (label == "rec_gain" || label == "main_volume")
            {
                auto pot = label == "rec_gain" ? mpc.getHardware()->getRecPot() : mpc.getHardware()->getVolPot();
                auto knob = dynamic_cast<Knob*>(event.eventComponent);
                pot->setValue(knob->getAngleFactor() * 100);
            }
            else if (label == "slider")
            {
                syncMpcSliderModelWithUi(event.eventComponent);
            }
        }

        void mouseDown(const juce::MouseEvent &e) override
        {
            setTooltipVisibility(e.eventComponent, false);

            if (label.length() >= 4 && label.substr(0, 4) == "pad-")
            {
                const auto digitsString = label.substr(4);
                const auto padNumber = std::stoi(digitsString);
                auto pad = mpc.getHardware()->getPad(padNumber - 1);
                const auto velocity = 127 - ((e.position.getY() / e.eventComponent->getBounds().getHeight()) * 127.f);
                pad->push(velocity);
                return;
            }
            else if (label == "cursor")
            {
                juce::Path left, top, bottom, right;

                left.startNewSubPath(0.f, 0.f);
                left.lineTo(0.2f, 0.f);
                left.lineTo(0.25f, 0.5f);
                left.lineTo(0.2f, 1.f);
                left.lineTo(0.f, 1.f);
                left.closeSubPath();

                top.startNewSubPath(0.2f, 0.f);
                top.lineTo(0.8f, 0.f);
                top.lineTo(0.75f, 0.5f);
                top.lineTo(0.25f, 0.5f);
                top.lineTo(0.2f, 0.f);
                top.closeSubPath();

                right = left;
                right.applyTransform(juce::AffineTransform(-1.0f, 0.0f, 1.f, 0.0f, 1.0f, 0.0f));

                bottom = top;
                bottom.applyTransform(juce::AffineTransform().verticalFlip(1.f));

                const auto compWidth = e.eventComponent->getWidth();
                const auto compHeight = e.eventComponent->getHeight();
                juce::AffineTransform scaleTransform = juce::AffineTransform().scaled(compWidth, compHeight);

                left.applyTransform(scaleTransform);
                top.applyTransform(scaleTransform);
                right.applyTransform(scaleTransform);
                bottom.applyTransform(scaleTransform);

                if (left.contains(e.position))
                {
                    mpc.getHardware()->getButton("left")->push();
                }
                else if (top.contains(e.position))
                {
                    mpc.getHardware()->getButton("up")->push();
                }
                else if (right.contains(e.position))
                {
                    mpc.getHardware()->getButton("right")->push();
                }
                else if (bottom.contains(e.position))
                {
                    mpc.getHardware()->getButton("down")->push();
                }

                return;
            }
            else if (label == "data-wheel" || label == "rec_gain" || label == "main_volume" || label == "slider")
            {
                return;
            }

            mpc.getHardware()->getButton(label)->push();
        }

        void mouseUp(const juce::MouseEvent &e) override
        {
            if (label.length() >= 4 && label.substr(0, 4) == "pad-")
            {
                const auto digitsString = label.substr(4);
                const auto padNumber = std::stoi(digitsString);
                auto pad = mpc.getHardware()->getPad(padNumber - 1);
                pad->release();
            }
            else if (label == "rec_gain" || label == "main_volume")
            {
            }
            else if (label == "cursor" || label == "slider")
            {
            }
            else if (label == "data-wheel")
            {
                lastDy = 0;
            }
            else
            {
                mpc.getHardware()->getButton(label)->release();
            }
        }

        void mouseDrag(const juce::MouseEvent &e) override
        {
            if (label == "rec_gain" || label == "main_volume")
            {
                auto pot = label == "rec_gain" ? mpc.getHardware()->getRecPot() : mpc.getHardware()->getVolPot();
                auto knob = dynamic_cast<Knob*>(e.eventComponent);
                pot->setValue(knob->getAngleFactor() * 100.f);
            }
            else if (label == "data-wheel")
            {
                auto dataWheel = mpc.getHardware()->getDataWheel();

                auto dY = -(e.getDistanceFromDragStartY() - lastDy);

                if (dY == 0)
                {
                    return;
                }

                dataWheel->turn(dY);

                lastDy = e.getDistanceFromDragStartY();
            }
            else if (label == "slider")
            {
                syncMpcSliderModelWithUi(e.eventComponent);
            }
        }

    private:
        int lastDy = 0;
        mpc::Mpc &mpc;
        const std::string label;
        vmpc_juce::gui::MouseWheelControllable mouseWheelControllable;

        void setTooltipVisibility(juce::Component *c, const bool visibleEnabled)
        {
            const auto editor = c->findParentComponentOfClass<juce::AudioProcessorEditor>();
            auto tooltipOverlay = getChildComponentOfClass<TooltipOverlay>(editor);
            tooltipOverlay->setTooltipVisibility(label, visibleEnabled);
        }

        void syncMpcSliderModelWithUi(juce::Component *eventComponent)
        {
            const auto sliderComponent = dynamic_cast<Slider*>(eventComponent);

            if (sliderComponent == nullptr)
            {
                return;
            }

            auto hwSlider = mpc.getHardware()->getSlider();
            const auto yPosFraction = sliderComponent->getSliderYPosFraction();
            hwSlider->setValue(yPosFraction * 127.f);
        }

};

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
    components.push_back(shadow);
    parent->addAndMakeVisible(shadow);

}

void ViewUtil::createComponent(
        mpc::Mpc &mpc,
        node &n,
        std::vector<juce::Component*> &components,
        juce::Component* parent,
        const std::function<float()> &getScale,
        const std::function<juce::Font&()> &getNimbusSansScaled,
        std::vector<juce::MouseListener*> &mouseListeners,
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
        createComponents(mpc, n, gridWrapper->components, gridWrapper, getScale, getNimbusSansScaled, mouseListeners, tooltipOverlay);
        components.push_back(gridWrapper);
        parent->addAndMakeVisible(gridWrapper);
        n.grid_wrapper_component = gridWrapper;
    }
    else if (n.node_type == "flex_box")
    {
        auto flexBoxWrapper = new FlexBoxWrapper(n, getScale);
        createComponents(mpc, n, flexBoxWrapper->components, flexBoxWrapper, getScale, getNimbusSansScaled, mouseListeners, tooltipOverlay);
        components.push_back(flexBoxWrapper);
        parent->addAndMakeVisible(flexBoxWrapper);
        n.flex_box_wrapper_component = flexBoxWrapper;
    }
    else if (n.node_type == "line_flanked_label")
    {
        auto lineFlankedLabel = new LineFlankedLabel(n.label, getScale, getNimbusSansScaled);
        components.push_back(lineFlankedLabel);
        parent->addAndMakeVisible(lineFlankedLabel);
        n.line_flanked_label_component = lineFlankedLabel;
    }
    else if (n.node_type == "j_shape" || n.node_type == "l_shape")
    {
        const auto jOrLShape = new JOrLShape(n.node_type == "j_shape" ? JOrLShape::Shape::J : JOrLShape::Shape::L, getScale);
        components.push_back(jOrLShape);
        parent->addAndMakeVisible(jOrLShape);
        n.j_or_l_shape_component = jOrLShape;
    }
    else if (n.node_type == "face_paint_grey_rectangle" || n.node_type == "chassis_rectangle" || n.node_type == "lcd_rectangle")
    {
        const auto rectangle = new Rectangle(n.node_type == "chassis_rectangle" ? Constants::chassisColour : n.node_type == "lcd_rectangle" ? Constants::lcdOff : Constants::greyFacePaintColour);
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
            if (doingTop) topLabel += c;
            else bottomLabel += c;
        }

        const auto numKey = new NumKey(getScale, topLabel, bottomLabel, n.svg, parent, n.shadow_size, getNimbusSansScaled);
        addShadow(n, getScale, numKey->getSvgComponent(), parent, components);
        components.push_back(numKey);
        parent->addAndMakeVisible(numKey);
        n.num_key_component = numKey;
        tooltipAnchor = numKey;
    }
    else if (n.node_type == "slider")
    {
        auto slider = new Slider(parent, getScale, n.shadow_size, getNimbusSansScaled);
        n.slider_component = slider;
        addShadow(n, getScale, slider->sliderCapSvg, parent, components);
        parent->addAndMakeVisible(n.slider_component);
        components.push_back(n.slider_component);

        const auto sliderValue = mpc.getHardware()->getSlider()->getValue();

        slider->setSliderYPosFraction(1.f - (sliderValue / 127.f));
    }
    else if (n.node_type == "data_wheel")
    {
        auto dataWheel = new DataWheel(parent, n.shadow_size, getScale);
        addShadow(n, getScale, dataWheel->backgroundSvg, parent, components);
        n.data_wheel_component = dataWheel;
        parent->addAndMakeVisible(dataWheel);
        components.push_back(dataWheel);
        tooltipAnchor = dataWheel;
    }
    else if (n.node_type == "lcd")
    {
        auto lcd = new Lcd(mpc);
        if (n.magic_multiplier > 0.f) lcd->magicMultiplier = n.magic_multiplier;
        n.lcd_component = lcd;
        parent->addAndMakeVisible(lcd);
        components.push_back(lcd);
    }
    else if (n.node_type == "red_led" || n.node_type == "green_led")
    {
        auto led = new Led(n.name, n.node_type == "red_led" ? Led::LedColor::RED : Led::LedColor::GREEN, getScale);
        n.led_component = led;
        parent->addAndMakeVisible(led);
        components.push_back(led);
    }
    else if (n.node_type == "pad")
    {
        assert(n.name.length() == 5 || n.name.length() == 6);
        const auto digitsString = n.name.substr(4);
        const auto padNumber = std::stoi(digitsString);
        auto hwPad = mpc.getHardware()->getPad(padNumber - 1);
        auto pad = new Pad(parent, n.shadow_size, getScale, mpc, hwPad);
        n.svg_component = pad;
        components.push_back(pad);
        addShadow(n, getScale, pad, parent, components);
        parent->addAndMakeVisible(pad);
        tooltipAnchor = pad;
    }
    else if (!n.svg.empty() && n.label.empty())
    {
        auto svgComponent = new SvgComponent({n.svg}, parent, n.shadow_size, getScale);
        components.push_back(svgComponent);
        addShadow(n, getScale, svgComponent, parent, components);
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
            svgComponent = new SvgComponent({n.svg}, parent, n.shadow_size, getScale);
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

            if (n.name != "rec_gain" && n.name != "main_volume")
            {
                tooltipAnchor = svgWithLabelGrid;
            }
        }
        else /* if parent is FlexBoxWrapper */
        {
            components.push_back(svgComponent);
            parent->addAndMakeVisible(svgComponent);

            components.push_back(labelComponent);
            parent->addAndMakeVisible(labelComponent);

            tooltipAnchor = svgComponent;
        }
    }
    else if (!n.label.empty())
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
    }

    if (!n.hardware_label.empty())
    {
        auto mouseListener = new MpcHardwareMouseListener(mpc, n.hardware_label);
        mouseListeners.push_back(mouseListener);

        for (auto it = components.rbegin(); it != components.rend(); ++it)
        {
            if (dynamic_cast<Shadow*>(*it) != nullptr)
            {
                continue;
            }

            (*it)->addMouseListener(mouseListener, true);

            if (n.name == "rec_gain" || n.name == "main_volume")
            {
                auto knob = dynamic_cast<Knob*>(n.svg_component);
                auto pot = n.name == "rec_gain" ? mpc.getHardware()->getRecPot() : mpc.getHardware()->getVolPot();
                knob->setAngleFactor(pot->getValue() * 0.01);
            }
            else if (auto dataWheelComponent = dynamic_cast<DataWheel*>(*it); dataWheelComponent != nullptr)
            {
                auto hwDataWheel = mpc.getHardware()->getDataWheel();
                hwDataWheel->updateUi = [dataWheelComponent](int increment) {
                    juce::MessageManager::callAsync([dataWheelComponent, increment] {
                            dataWheelComponent->setAngle(dataWheelComponent->getAngle() + (increment * 0.02f));
                            });
                };

            }
            break;
        }

        if (tooltipAnchor != nullptr)
        {
            const auto getTooltipText = [&mpc, &n]{
                const auto kbMapping = mpc.getControls()->getKbMapping().lock();
                if (n.node_type == "data_wheel")
                {
                    const auto decrement = kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel("datawheel-down"), true);
                    const auto increment = kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel("datawheel-up"), true);
                    return decrement + " / " + increment;
                }
                else if (n.hardware_label == "cursor")
                {
                    const auto left = kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel("left"), true);
                    const auto right = kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel("right"), true);
                    const auto up = kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel("up"), true);
                    const auto down = kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel("down"), true);
                    return left + " / " + right + " / " + up + " / " + down;
                }

                const auto keyboardMappingText = kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel(n.hardware_label), true);
                return keyboardMappingText;
            };

            const auto tooltip = new Tooltip(getTooltipText, tooltipAnchor, getNimbusSansScaled, getScale, n.hardware_label);
            components.push_back(tooltip);
            tooltipOverlay->addChildComponent(tooltip);
        }
    }
}

void ViewUtil::createComponents(
        mpc::Mpc &mpc,
        node &n,
        std::vector<juce::Component*> &components,
        juce::Component *parent,
        const std::function<float()> &getScale,
        const std::function<juce::Font&()> &getNimbusSansScaled,
        std::vector<juce::MouseListener*> &mouseListeners,
        juce::Component *tooltipOverlay)
{
    for (auto& c : n.children)
    {
        createComponent(mpc, c, components, parent, getScale, getNimbusSansScaled, mouseListeners, tooltipOverlay);
    }
}

