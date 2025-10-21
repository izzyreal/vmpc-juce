#pragma once

#include "input/HostInputEvent.hpp"

#include "hardware/ComponentId.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <string>
#include <optional>

using HostInputEvent = mpc::input::HostInputEvent;
using GestureEvent = mpc::input::GestureEvent;

static mpc::hardware::ComponentId getCursorComponentId(const juce::MouseEvent &e)
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
    juce::AffineTransform scaleTransform = juce::AffineTransform().scaled(static_cast<float>(compWidth), static_cast<float>(compHeight));

    left.applyTransform(scaleTransform);
    top.applyTransform(scaleTransform);
    right.applyTransform(scaleTransform);
    bottom.applyTransform(scaleTransform);

    if (left.contains(e.position))
    {
        return mpc::hardware::ComponentId::CURSOR_LEFT_OR_DIGIT;
    }
    else if (top.contains(e.position))
    {
        return mpc::hardware::ComponentId::CURSOR_UP;
    }
    else if (right.contains(e.position))
    {
        return mpc::hardware::ComponentId::CURSOR_RIGHT_OR_DIGIT;
    }
    else if (bottom.contains(e.position))
    {
        return mpc::hardware::ComponentId::CURSOR_DOWN;
    }

    return mpc::hardware::ComponentId::NONE;
}


static std::optional<HostInputEvent> makeAbsoluteGestureFromMouse(
    const juce::MouseEvent& e,
    const std::string& label,
    GestureEvent::Type type,
    std::optional<float> customNormY)
{
    if (!mpc::hardware::componentLabelToId.count(label) && label != "cursor")
    {
        return std::nullopt;
    }

    mpc::hardware::ComponentId componentId;

    if (label == "cursor")
    {
        componentId = getCursorComponentId(e);
    }
    else
    {
        componentId = mpc::hardware::componentLabelToId.at(label);
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
            e.getNumberOfClicks(),
            componentId
        }
    };
}

static std::optional<mpc::input::HostInputEvent> makeRelativeGestureFromMouse(const juce::MouseEvent &e,
    const std::string& label,
    GestureEvent::Type type,
    float continuousDelta)
{
    using namespace mpc::input;

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
            e.getNumberOfClicks(),
            mpc::hardware::componentLabelToId.at(label)
        }
    };
}

