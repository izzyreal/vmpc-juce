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

void MpcHardwareMouseListener::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &)
{
    setKeyTooltipVisibility(event.eventComponent, false);

    if (auto hostInputEvent = constructHostInputEventFromJuceMouseEvent(event, label, mpc::inputlogic::MouseEvent::MouseEventType::WHEEL); hostInputEvent.has_value())
    {
        mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
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

    if (auto hostInputEvent = constructHostInputEventFromJuceMouseEvent(e, label, mpc::inputlogic::MouseEvent::MouseEventType::BUTTON_DOWN); hostInputEvent.has_value())
    {
        mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
    }
}

void MpcHardwareMouseListener::mouseDoubleClick(const juce::MouseEvent &)
{
    if (label.length() >= 5 && label.substr(0, 5) == componentIdToLabel.at(ComponentId::SHIFT))
    {
        showKeyTooltipUponNextClick = true;
    }
}

void MpcHardwareMouseListener::mouseUp(const juce::MouseEvent &e)
{
    if (auto hostInputEvent = constructHostInputEventFromJuceMouseEvent(e, label, mpc::inputlogic::MouseEvent::MouseEventType::BUTTON_UP); hostInputEvent.has_value())
    {
        mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
    }
}

void MpcHardwareMouseListener::mouseDrag(const juce::MouseEvent &e)
{
    if (auto hostInputEvent = constructHostInputEventFromJuceMouseEvent(e, label, mpc::inputlogic::MouseEvent::MouseEventType::DRAG); hostInputEvent.has_value())
    {
        mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
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

