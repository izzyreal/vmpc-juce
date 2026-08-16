#pragma once

#include <input/HostInputEvent.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

namespace mpc
{
    class Mpc;
}

namespace vmpc_juce::gui
{
    struct LcdPointerEvent
    {
        juce::Point<float> position;
        int sourceIndex = 0;
        mpc::input::GestureEvent::InputDeviceType deviceType =
            mpc::input::GestureEvent::InputDeviceType::Mouse;
        bool isPrimary = false;
        bool shiftDown = false;
        bool ctrlDown = false;
        bool altDown = false;
    };

    LcdPointerEvent makeLcdPointerEvent(const juce::MouseEvent &event);

    std::optional<mpc::input::HostInputEvent>
    makeLcdHostInputEvent(const LcdPointerEvent &event,
                          juce::Rectangle<float> renderedLcdBounds,
                          mpc::input::GestureEvent::Type type);

    std::optional<mpc::input::HostInputEvent>
    makeLcdWheelHostInputEvent(const LcdPointerEvent &event,
                               juce::Rectangle<float> renderedLcdBounds,
                               float continuousDelta);

    mpc::input::HostInputResult
    dispatchLcdPointerEvent(mpc::Mpc &mpc, const juce::MouseEvent &event,
                            juce::Rectangle<float> renderedLcdBounds,
                            mpc::input::GestureEvent::Type type);

    mpc::input::HostInputResult
    dispatchLcdWheelEvent(mpc::Mpc &mpc, const juce::MouseEvent &event,
                          const juce::MouseWheelDetails &wheel,
                          juce::Rectangle<float> renderedLcdBounds);
} // namespace vmpc_juce::gui
