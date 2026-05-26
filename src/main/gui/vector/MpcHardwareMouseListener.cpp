#include "MpcHardwareMouseListener.hpp"

#include "gui/vector/MpcInputUtil.hpp"
#include "gui/vector/SvgComponent.hpp"
#include "gui/vector/TooltipOverlay.hpp"
#include "utils/ComponentUtils.hpp"

#include <Mpc.hpp>

#include <hardware/ComponentId.hpp>
#include <input/HostInputEvent.hpp>

#include <juce_audio_processors/juce_audio_processors.h>

using namespace vmpc_juce::gui::vector;
using namespace mpc::hardware;

namespace
{
    SvgComponent *findSvgComponentRecursive(juce::Component *component)
    {
        if (component == nullptr)
        {
            return nullptr;
        }

        if (auto *svgComponent = dynamic_cast<SvgComponent *>(component))
        {
            return svgComponent;
        }

        for (auto *child : component->getChildren())
        {
            if (auto *svgComponent = findSvgComponentRecursive(child))
            {
                return svgComponent;
            }
        }

        return nullptr;
    }

    SvgComponent *findSvgComponentForMouseEvent(const juce::MouseEvent &e)
    {
        for (auto *component = e.eventComponent; component != nullptr;
             component = component->getParentComponent())
        {
            if (auto *svgComponent = findSvgComponentRecursive(component))
            {
                return svgComponent;
            }
        }

        return nullptr;
    }
} // namespace

bool MpcHardwareMouseListener::showKeyTooltipUponNextClick = false;

MpcHardwareMouseListener::MpcHardwareMouseListener(
    mpc::Mpc &mpcToUse, const std::string &labelToUse)
    : mpc(mpcToUse), label(labelToUse)
{
}

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
        if (isTimerRunning())
        {
            stopTimer();
        }
        return;
    }

    lastEventComponent = e.eventComponent;
    if (!isTimerRunning())
    {
        startTimer(1000);
    }
}

void MpcHardwareMouseListener::mouseExit(const juce::MouseEvent &)
{
    hideKeyTooltipUntilAfterMouseExit = false;
    if (isTimerRunning())
    {
        stopTimer();
    }
    if (lastEventComponent != nullptr)
    {
        setKeyTooltipVisibility(lastEventComponent, false);
    }
    lastEventComponent = nullptr;
}

void MpcHardwareMouseListener::mouseWheelMove(
    const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel)
{
    hideKeyTooltipUntilAfterMouseExit = true;
    setKeyTooltipVisibility(event.eventComponent, false);

    float sensitivity = 10.0f;

    if (wheel.isSmooth)
    {
        sensitivity *= 4.0f; // boost for touch/trackpad
    }

    if (wheel.isInertial)
    {
        sensitivity *= 2.0f; // boost inertial motion a bit too
    }

    const float continuousDelta = -wheel.deltaY * sensitivity;

    if (const auto hostInputEvent = makeRelativeGestureFromMouse(
            event, label, GestureEvent::Type::UPDATE, continuousDelta);
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

    previousDragY[e.source.getIndex()] = e.position.getY();

    if (auto *svgComponent = findSvgComponentForMouseEvent(e))
    {
        svgComponent->setPressedAppearanceEnabled(true);
    }

    const auto gestureType = e.getNumberOfClicks() >= 2
                                 ? GestureEvent::Type::REPEAT
                                 : GestureEvent::Type::BEGIN;

    if (const auto hostInputEvent =
            makeAbsoluteGestureFromMouse(e, label, gestureType, std::nullopt);
        hostInputEvent)
    {
        mpc.dispatchHostInput(*hostInputEvent);
    }
}

void MpcHardwareMouseListener::mouseDoubleClick(const juce::MouseEvent &)
{
    if (label.length() >= 5 &&
        label.substr(0, 5) == componentIdToLabel.at(SHIFT))
    {
        showKeyTooltipUponNextClick = true;
    }
}

void MpcHardwareMouseListener::mouseUp(const juce::MouseEvent &e)
{
    if (auto *svgComponent = findSvgComponentForMouseEvent(e))
    {
        svgComponent->setPressedAppearanceEnabled(false);
    }

    if (const auto hostInputEvent = makeAbsoluteGestureFromMouse(
            e, label, mpc::input::GestureEvent::Type::END, std::nullopt);
        hostInputEvent)
    {
        mpc.dispatchHostInput(*hostInputEvent);
    }

    previousDragY.erase(e.source.getIndex());
}

void MpcHardwareMouseListener::mouseDrag(const juce::MouseEvent &e)
{
    hideKeyTooltipUntilAfterMouseExit = true;
    setKeyTooltipVisibility(e.eventComponent, false);

    if (label == "slider")
    {
        // The slider handles drag events itself
        return;
    }

    const float deltaY = previousDragY[e.source.getIndex()] - e.position.getY();
    previousDragY[e.source.getIndex()] = e.position.getY();

    if (deltaY != 0.0f)
    {
        if (const auto hostInputEvent = makeRelativeGestureFromMouse(
                e, label, mpc::input::GestureEvent::Type::UPDATE, deltaY);
            hostInputEvent)
        {
            mpc.dispatchHostInput(*hostInputEvent);
        }
    }
}

void MpcHardwareMouseListener::setKeyTooltipVisibility(
    const juce::Component *c, const bool visibleEnabled) const
{
    const auto editor =
        c->findParentComponentOfClass<juce::AudioProcessorEditor>();

    const auto tooltipOverlay =
        vmpc_juce::utils::findFirstChildComponentOfClass<TooltipOverlay>(
            editor);

    if (tooltipOverlay == nullptr)
    {
        return;
    }

    tooltipOverlay->setKeyTooltipVisibility(label, visibleEnabled);
}

bool MpcHardwareMouseListener::isPad() const
{
    return label.length() >= 4 && label.substr(0, 4) == "pad-";
}
