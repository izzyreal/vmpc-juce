#include "MpcHardwareMouseListener.hpp"

#include "MpcInputUtil.h"
#include "TooltipOverlay.hpp"

#include "hardware2/ComponentIdLabelMap.h"
#include "inputlogic/HostInputEvent.h"
#include "hardware2/Hardware2.h"

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

    // Keep an accumulator as a class member: float wheelAccumulator = 0.f;
    float sensitivity = 10.0f;

    // macOS often gives tiny deltas for smooth/inertial scrolling
    if (wheel.isSmooth)
        sensitivity *= 4.0f;   // boost for touch/trackpad
    if (wheel.isInertial)
        sensitivity *= 2.0f;   // boost inertial motion a bit too

    wheelAccumulator += -wheel.deltaY * sensitivity;

    int stepDelta = static_cast<int>(wheelAccumulator);
    if (stepDelta != 0)
    {
        wheelAccumulator -= stepDelta; // preserve remainder

        if (auto hostInputEvent = constructHostInputEventFromJuceMouseEvent(
                event, label, mpc::inputlogic::GestureEvent::Type::UPDATE, stepDelta);
            hostInputEvent.has_value())
        {
            mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
        }
    }
}

void MpcHardwareMouseListener::mouseDown(const juce::MouseEvent &e)
{
    printf("MpcHardwareMouseListener mouseDown\n");
    if (showKeyTooltipUponNextClick)
    {
        setKeyTooltipVisibility(e.eventComponent, true);
        showKeyTooltipUponNextClick = false;
        return;
    }

    setKeyTooltipVisibility(e.eventComponent, false);
    hideKeyTooltipUntilAfterMouseExit = true;

    previousDragY = e.position.getY();
    dragYAccumulator = 0.f;

    if (auto hostInputEvent = constructHostInputEventFromJuceMouseEvent(e, label, mpc::inputlogic::GestureEvent::Type::BEGIN);
        hostInputEvent.has_value())
    {
        mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
    }
}

void MpcHardwareMouseListener::mouseDoubleClick(const juce::MouseEvent &e)
{
    printf("MpcHardwareMouseListener mouseDoubleClick\n");

    if (label.length() >= 5 && label.substr(0, 5) == componentIdToLabel.at(ComponentId::SHIFT))
    {
        showKeyTooltipUponNextClick = true;
    }

    if (auto hostInputEvent = constructHostInputEventFromJuceMouseEvent(e, label, mpc::inputlogic::GestureEvent::Type::REPEAT);
        hostInputEvent.has_value())
    {
        mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
    }
}

void MpcHardwareMouseListener::mouseUp(const juce::MouseEvent &e)
{
    printf("MpcHardwareMouseListener mouseUp\n");

    if (auto hostInputEvent = constructHostInputEventFromJuceMouseEvent(e, label, mpc::inputlogic::GestureEvent::Type::END);
        hostInputEvent.has_value())
    {
        mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
    }
}

void MpcHardwareMouseListener::mouseDrag(const juce::MouseEvent &e)
{
    const float deltaY = previousDragY - e.position.getY();
    dragYAccumulator += deltaY;

    constexpr float stepThreshold = 4.0f; // pixels per tick
    int stepDelta = 0;

    if (std::abs(dragYAccumulator) >= stepThreshold)
    {
        stepDelta = static_cast<int>(dragYAccumulator / stepThreshold);
        dragYAccumulator -= stepDelta * stepThreshold; // preserve remainder
    }

    if (stepDelta != 0)
    {
        if (auto hostInputEvent = constructHostInputEventFromJuceMouseEvent(
                e, label, mpc::inputlogic::GestureEvent::Type::UPDATE, stepDelta);
            hostInputEvent.has_value())
        {
            mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
        }
    }

    previousDragY = e.position.getY();
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

