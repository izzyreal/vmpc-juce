#include "MpcHardwareMouseListener.hpp"

#include "hardware2/Hardware2.h"

#include "Knob.hpp"

#include "TooltipOverlay.hpp"
#include "Slider.hpp"
#include "juce_audio_processors/juce_audio_processors.h"

using namespace vmpc_juce::gui::vector;

bool MpcHardwareMouseListener::showKeyTooltipUponNextClick = false;

MpcHardwareMouseListener::MpcHardwareMouseListener(mpc::Mpc &mpcToUse, const std::string labelToUse) 
    : mpc(mpcToUse), label(labelToUse) {}

void MpcHardwareMouseListener::timerCallback()
{
    if (!hideKeyTooltipUntilAfterMouseExit && lastEventComponent != nullptr)
    {
        setKeyTooltipVisibility(lastEventComponent, true);
    }
}

void MpcHardwareMouseListener::mouseMove(const juce::MouseEvent &e)
{
    const auto currentModifiers = juce::ModifierKeys::getCurrentModifiers();

    if (!currentModifiers.isAnyModifierKeyDown())
    {
        if (isTimerRunning()) stopTimer();
        return;
    }

    lastEventComponent = e.eventComponent;
    if (!isTimerRunning()) startTimer(1000);
}

void MpcHardwareMouseListener::mouseExit(const juce::MouseEvent &)
{
    hideKeyTooltipUntilAfterMouseExit = false;
    if (isTimerRunning()) stopTimer();
    if (lastEventComponent != nullptr)
    {
        setKeyTooltipVisibility(lastEventComponent, false);
    }
    lastEventComponent = nullptr;
}

void MpcHardwareMouseListener::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &)
{
    setKeyTooltipVisibility(event.eventComponent, false);

    if (label == "rec_gain" || label == "main_volume")
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

void MpcHardwareMouseListener::pushHardware(const juce::MouseEvent &e)
{
    using namespace mpc::inputlogic;

    HostInputEvent hostEvent;
    hostEvent.source = HostInputEvent::MOUSE;

    const float normX = e.position.getX() / static_cast<float>(e.eventComponent->getWidth());
    const float normY = e.position.getY() / static_cast<float>(e.eventComponent->getHeight());

    hostEvent.payload = MouseEvent {
        MouseEvent::ButtonState{
            e.mods.isLeftButtonDown(),
            e.mods.isMiddleButtonDown(),
            e.mods.isRightButtonDown()
        },
        MouseEvent::NONE,
        normX,
        normY,
        0.f,
        0.f,
        0.f,
        MouseEvent::BUTTON_DOWN
    };

    MouseEvent& mouseEvent = std::get<MouseEvent>(hostEvent.payload);

    if (isPad())
    {
        const auto digitsString = label.substr(4);
        const auto padNumber = std::stoi(digitsString);
        mouseEvent.guiElement = static_cast<MouseEvent::GuiElement>(MouseEvent::PAD1 + padNumber - 1);
        mpc.getHardware2()->dispatchHostInput(hostEvent);
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
            mouseEvent.guiElement = MouseEvent::CURSOR_LEFT;
        }
        else if (top.contains(e.position))
        {
            mouseEvent.guiElement = MouseEvent::CURSOR_UP;
        }
        else if (right.contains(e.position))
        {
            mouseEvent.guiElement = MouseEvent::CURSOR_RIGHT;
        }
        else if (bottom.contains(e.position))
        {
            mouseEvent.guiElement = MouseEvent::CURSOR_DOWN;
        }

        mpc.getHardware2()->dispatchHostInput(hostEvent);
        return;
    }
    else if (label == "data-wheel" || label == "rec_gain" || label == "main_volume" || label == "slider")
    {
        return;
    }

    mpc.getHardware()->getButton(label)->push();
}

void MpcHardwareMouseListener::mouseDown(const juce::MouseEvent &e)
{
    if (showKeyTooltipUponNextClick)
    {
        setKeyTooltipVisibility(e.eventComponent, true);
        showKeyTooltipUponNextClick = false;
        return;
    }

    setKeyTooltipVisibility(e.eventComponent, false);
    hideKeyTooltipUntilAfterMouseExit = true;
    pushHardware(e);
}

void MpcHardwareMouseListener::mouseDoubleClick(const juce::MouseEvent &)
{
    if (label.length() >= 5 && label.substr(0, 5) == "shift")
    {
        showKeyTooltipUponNextClick = true;
    }
}

void MpcHardwareMouseListener::mouseUp(const juce::MouseEvent &)
{
    if (label.length() >= 4 && label.substr(0, 4) == "pad-")
    {
        const auto digitsString = label.substr(4);
        const auto padNumber = std::stoi(digitsString);
        auto mpcPad = mpc.getHardware2()->getPad(padNumber - 1);
        mpcPad->release();
    }
    else if (label != "rec_gain" && label != "main_volume" && label != "cursor" && label != "slider" && label != "data-wheel")
    {
        mpc.getHardware()->getButton(label)->release();
    }
}

void MpcHardwareMouseListener::mouseDrag(const juce::MouseEvent &e)
{
    if (label == "rec_gain" || label == "main_volume")
    {
        auto pot = label == "rec_gain" ? mpc.getHardware()->getRecPot() : mpc.getHardware()->getVolPot();
        auto knob = dynamic_cast<Knob*>(e.eventComponent);
        pot->setValue(knob->getAngleFactor() * 100.f);
    }
    else if (label == "slider")
    {
        syncMpcSliderModelWithUi(e.eventComponent);
    }
}

void MpcHardwareMouseListener::setKeyTooltipVisibility(juce::Component *c, const bool visibleEnabled)
{
    const auto editor = c->findParentComponentOfClass<juce::AudioProcessorEditor>();
    auto tooltipOverlay = getChildComponentOfClass<TooltipOverlay>(editor);
    
    if (tooltipOverlay == nullptr)
    {
        return;
    }
    
    tooltipOverlay->setKeyTooltipVisibility(label, visibleEnabled);
}
void MpcHardwareMouseListener::syncMpcSliderModelWithUi(juce::Component *eventComponent)
{
    const auto sliderComponent = dynamic_cast<Slider*>(eventComponent);

    if (sliderComponent == nullptr)
    {
        return;
    }

    auto hwSlider = mpc.getHardware()->getSlider();
    const auto yPosFraction = sliderComponent->getSliderYPosFraction();
    hwSlider->setValue(127 - (yPosFraction * 127.f));
}

bool MpcHardwareMouseListener::isPad()
{
    return label.length() >= 4 && label.substr(0, 4) == "pad-";
}


