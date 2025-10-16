#include "MpcHardwareMouseListener.hpp"

#include "MpcInputUtil.h"
#include "TooltipOverlay.hpp"

#include "hardware2/ComponentIdLabelMap.h"
#include "inputlogic/HostInputEvent.h"

#include "juce_audio_processors/juce_audio_processors.h"

using namespace vmpc_juce::gui::vector;
using namespace mpc::hardware2;

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

void MpcHardwareMouseListener::mouseWheelMove(const juce::MouseEvent &event,
                                              const juce::MouseWheelDetails &wheel)
{
    setKeyTooltipVisibility(event.eventComponent, false);

    float sensitivity = 10.0f;

    if (wheel.isSmooth)
    {
        sensitivity *= 4.0f;   // boost for touch/trackpad
    }

    if (wheel.isInertial)
    {
        sensitivity *= 2.0f;   // boost inertial motion a bit too
    }

    const float continuousDelta = -wheel.deltaY * sensitivity;

    if (auto hostInputEvent = makeRelativeGestureFromMouse(label, GestureEvent::Type::UPDATE, continuousDelta);
        hostInputEvent)
    {
        mpc.dispatchHostInput(*hostInputEvent);
    }
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

    previousDragY = e.position.getY();

    const auto gestureType = e.getNumberOfClicks() >= 2 ? GestureEvent::Type::REPEAT : GestureEvent::Type::BEGIN;

    if (auto hostInputEvent = makeAbsoluteGestureFromMouse(e, label, gestureType, std::nullopt);
        hostInputEvent)
    {
        mpc.dispatchHostInput(*hostInputEvent);
    }
}

void MpcHardwareMouseListener::mouseDoubleClick(const juce::MouseEvent&)
{
    if (label.length() >= 5 && label.substr(0, 5) == componentIdToLabel.at(ComponentId::SHIFT))
    {
        showKeyTooltipUponNextClick = true;
    }
}

void MpcHardwareMouseListener::mouseUp(const juce::MouseEvent &e)
{
    if (auto hostInputEvent = makeAbsoluteGestureFromMouse(e, label, mpc::inputlogic::GestureEvent::Type::END, std::nullopt);
        hostInputEvent)
    {
        mpc.dispatchHostInput(*hostInputEvent);
    }
}

void MpcHardwareMouseListener::mouseDrag(const juce::MouseEvent &e)
{
    if (label == "slider")
    {
        // The slider handles drag events itself
        return;
    }

    const float deltaY = previousDragY - e.position.getY();
    previousDragY = e.position.getY();

    if (deltaY != 0.0f)
    {
        if (auto hostInputEvent = makeRelativeGestureFromMouse(
                label,
                mpc::inputlogic::GestureEvent::Type::UPDATE,
                deltaY);
            hostInputEvent)
        {
            mpc.dispatchHostInput(*hostInputEvent);
        }
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

bool MpcHardwareMouseListener::isPad()
{
    return label.length() >= 4 && label.substr(0, 4) == "pad-";
}

