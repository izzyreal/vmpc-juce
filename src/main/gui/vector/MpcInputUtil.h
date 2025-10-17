#pragma once

#include "inputlogic/HostInputEvent.h"

#include "hardware/ComponentId.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <string>
#include <optional>

using HostInputEvent = mpc::inputlogic::HostInputEvent;
using GestureEvent = mpc::inputlogic::GestureEvent;

static std::optional<HostInputEvent> makeAbsoluteGestureFromMouse(
    const juce::MouseEvent& e,
    const std::string& label,
    GestureEvent::Type type,
    std::optional<float> customNormY)
{
    if (!mpc::hardware::componentLabelToId.count(label))
    {
        return std::nullopt;
    }

    const float normY = customNormY.has_value()
        ? *customNormY
        : juce::jlimit(0.0f, 1.0f, (float)e.position.getY() / (float)e.eventComponent->getHeight());

    return HostInputEvent{
        GestureEvent{
            type,
            GestureEvent::Movement::Absolute,
            normY,
            0.f,
            0,
            mpc::hardware::componentLabelToId.at(label)
        }
    };
}

static std::optional<mpc::inputlogic::HostInputEvent> makeRelativeGestureFromMouse(const juce::MouseEvent &e,
    const std::string& label,
    GestureEvent::Type type,
    float continuousDelta)
{
    using namespace mpc::inputlogic;

    if (!mpc::hardware::componentLabelToId.count(label))
    {
        return std::nullopt;
    }
    const float normY = juce::jlimit(0.0f, 1.0f, (float)e.position.getY() / (float)e.eventComponent->getHeight());

    return HostInputEvent{
        GestureEvent{
            type,
            GestureEvent::Movement::Relative,
            normY,
            continuousDelta,
            0,
            mpc::hardware::componentLabelToId.at(label)
        }
    };
}

/*
static std::optional<mpc::inputlogic::HostInputEvent> constructHostInputEventFromJuceMouseEvent(const juce::MouseEvent &e,
                                                                              std::string label,
                                                                              mpc::inputlogic::GestureEvent::Type gestureEventType,
                                                                              const int discreteDelta = 0,
                                                                              const float continuousDelta = 0.f,
                                                                              const std::optional<float> customNormY = std::nullopt)
{
    if (label.empty())
    {
        return std::nullopt;
    }

    using namespace mpc::inputlogic;

    const float normY = customNormY.has_value() ? *customNormY : e.position.getY() / static_cast<float>(e.eventComponent->getHeight());

    mpc::inputlogic::GestureEvent gestureEvent {
        gestureEventType,
        normY,
        discreteDelta,
        continuousDelta,
        gestureEventType == GestureEvent::Type::REPEAT ? 2 : 0,
        mpc::hardware::ComponentId::NONE
    };

    if (mpc::hardware::componentLabelToId.count(label) > 0)
    {
        gestureEvent.componentId = mpc::hardware::componentLabelToId.at(label);
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
            gestureEvent.componentId = mpc::hardware::ComponentId::CURSOR_LEFT;
        }
        else if (top.contains(e.position))
        {
            gestureEvent.componentId = mpc::hardware::ComponentId::CURSOR_UP;
        }
        else if (right.contains(e.position))
        {
            gestureEvent.componentId = mpc::hardware::ComponentId::CURSOR_RIGHT;
        }
        else if (bottom.contains(e.position))
        {
            gestureEvent.componentId = mpc::hardware::ComponentId::CURSOR_DOWN;
        }
    }

    return HostInputEvent(gestureEvent);
}
*/
