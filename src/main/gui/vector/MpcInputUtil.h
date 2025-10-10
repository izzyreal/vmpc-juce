#pragma once

#include "inputlogic/HostInputEvent.h"

#include "hardware2/ComponentIdLabelMap.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <string>
#include <optional>

static std::optional<mpc::inputlogic::HostInputEvent> constructHostInputEventFromJuceMouseEvent(const juce::MouseEvent &e,
                                                                              std::string label,
                                                                              mpc::inputlogic::MouseEvent::MouseEventType mouseEventType)
{
    if (label.empty())
    {
        return std::nullopt;
    }

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
        mpc::hardware2::ComponentId::NONE,
        normX,
        normY,
        0.f,
        0.f,
        0.f,
        mouseEventType
    };

    MouseEvent& mouseEvent = std::get<MouseEvent>(hostEvent.payload);

    if (mpc::hardware2::componentLabelToId.count(label) > 0)
    {
        mouseEvent.componentId = mpc::hardware2::componentLabelToId.at(label);
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
            mouseEvent.componentId = mpc::hardware2::ComponentId::CURSOR_LEFT;
        }
        else if (top.contains(e.position))
        {
            mouseEvent.componentId = mpc::hardware2::ComponentId::CURSOR_UP;
        }
        else if (right.contains(e.position))
        {
            mouseEvent.componentId = mpc::hardware2::ComponentId::CURSOR_RIGHT;
        }
        else if (bottom.contains(e.position))
        {
            mouseEvent.componentId = mpc::hardware2::ComponentId::CURSOR_DOWN;
        }
    }

    return hostEvent;
}

