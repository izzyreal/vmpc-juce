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

#include "hardware/Hardware.hpp"
#include "hardware/Button.hpp"
#include "hardware/HwPad.hpp"

using namespace vmpc_juce::gui::vector;

class MpcHardwareMouseListener : public juce::MouseListener {
    public:
        MpcHardwareMouseListener(mpc::Mpc &mpcToUse, const std::string labelToUse) : label(labelToUse), mpc(mpcToUse) {}

        void mouseDown(const juce::MouseEvent &e) override
        {
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
                return;
            }
            else if (label == "cursor")
            {
                return;
            }

            mpc.getHardware()->getButton(label)->release();
        }

    private:
        mpc::Mpc &mpc;
        const std::string label;
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
        std::vector<juce::MouseListener*> &mouseListeners)
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
        createComponents(mpc, n, gridWrapper->components, gridWrapper, getScale, getNimbusSansScaled, mouseListeners);
        components.push_back(gridWrapper);
        parent->addAndMakeVisible(gridWrapper);
        n.grid_wrapper_component = gridWrapper;
    }
    else if (n.node_type == "flex_box")
    {
        auto flexBoxWrapper = new FlexBoxWrapper(n, getScale);
        createComponents(mpc, n, flexBoxWrapper->components, flexBoxWrapper, getScale, getNimbusSansScaled, mouseListeners);
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

    }
    else if (n.node_type == "slider")
    {
        auto slider = new Slider(parent, getScale, n.shadow_size, getNimbusSansScaled);
        n.slider_component = slider;
        addShadow(n, getScale, slider->sliderCapSvg, parent, components);
        parent->addAndMakeVisible(n.slider_component);
        components.push_back(n.slider_component);
    }
    else if (n.node_type == "data_wheel")
    {
        auto dataWheel = new DataWheel(parent, n.shadow_size, getScale);
        addShadow(n, getScale, dataWheel->backgroundSvg, parent, components);
        n.data_wheel_component = dataWheel;
        parent->addAndMakeVisible(dataWheel);
        components.push_back(dataWheel);
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
        auto led = new Led(n.node_type == "red_led" ? Led::LedColor::RED : Led::LedColor::GREEN, getScale);
        n.led_component = led;
        parent->addAndMakeVisible(led);
        components.push_back(led);
    }
    else if (!n.svg.empty() && n.label.empty())
    {
        auto svgComponent = new SvgComponent(n.svg, parent, n.shadow_size, getScale);
        components.push_back(svgComponent);
        addShadow(n, getScale, svgComponent, parent, components);
        parent->addAndMakeVisible(svgComponent);
        n.svg_component = svgComponent;

        if (n.hide_svg)
        {
            svgComponent->setVisible(false);
        }
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
            break;
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
        std::vector<juce::MouseListener*> &mouseListeners)
{
    for (auto& c : n.children)
    {
        createComponent(mpc, c, components, parent, getScale, getNimbusSansScaled, mouseListeners);
    }
}

