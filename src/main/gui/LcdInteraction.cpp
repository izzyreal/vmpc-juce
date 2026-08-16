#include "gui/LcdInteraction.hpp"

#include <hardware/ComponentId.hpp>

#include <cmath>

namespace vmpc_juce::gui
{
    LcdPointerEvent makeLcdPointerEvent(const juce::MouseEvent &event)
    {
        const auto deviceType =
            event.source.isPen()
                ? mpc::input::GestureEvent::InputDeviceType::Pen
            : event.source.isTouch()
                ? mpc::input::GestureEvent::InputDeviceType::Touch
                : mpc::input::GestureEvent::InputDeviceType::Mouse;
        return {event.position,
                event.source.getIndex(),
                deviceType,
                event.source.isTouch() || event.source.isPen() ||
                    event.mods.isLeftButtonDown(),
                event.mods.isShiftDown(),
                event.mods.isCtrlDown(),
                event.mods.isAltDown()};
    }

    std::optional<mpc::input::HostInputEvent>
    makeLcdHostInputEvent(const LcdPointerEvent &event,
                          const juce::Rectangle<float> renderedLcdBounds,
                          const mpc::input::GestureEvent::Type type)
    {
        if (renderedLcdBounds.isEmpty() || !std::isfinite(event.position.x) ||
            !std::isfinite(event.position.y) ||
            type == mpc::input::GestureEvent::Type::REPEAT ||
            (type == mpc::input::GestureEvent::Type::BEGIN &&
             (!event.isPrimary || !renderedLcdBounds.contains(event.position))))
        {
            return std::nullopt;
        }

        const auto normX = (event.position.x - renderedLcdBounds.getX()) /
                           renderedLcdBounds.getWidth();
        const auto normY = (event.position.y - renderedLcdBounds.getY()) /
                           renderedLcdBounds.getHeight();

        return mpc::input::HostInputEvent(mpc::input::GestureEvent{
            type, mpc::input::GestureEvent::Movement::Absolute, normX, normY,
            0.f, 0, event.sourceIndex, mpc::hardware::ComponentId::LCD,
            event.shiftDown, event.ctrlDown, event.altDown, event.deviceType});
    }

    std::optional<mpc::input::HostInputEvent>
    makeLcdWheelHostInputEvent(const LcdPointerEvent &event,
                               const juce::Rectangle<float> renderedLcdBounds,
                               const float continuousDelta)
    {
        if (renderedLcdBounds.isEmpty() ||
            !renderedLcdBounds.contains(event.position) ||
            !std::isfinite(event.position.x) ||
            !std::isfinite(event.position.y) ||
            !std::isfinite(continuousDelta) || continuousDelta == 0.f)
        {
            return std::nullopt;
        }

        const auto normX = (event.position.x - renderedLcdBounds.getX()) /
                           renderedLcdBounds.getWidth();
        const auto normY = (event.position.y - renderedLcdBounds.getY()) /
                           renderedLcdBounds.getHeight();

        return mpc::input::HostInputEvent(mpc::input::GestureEvent{
            mpc::input::GestureEvent::Type::UPDATE,
            mpc::input::GestureEvent::Movement::Relative, normX, normY,
            continuousDelta, 0, event.sourceIndex,
            mpc::hardware::ComponentId::LCD, event.shiftDown, event.ctrlDown,
            event.altDown, event.deviceType});
    }

} // namespace vmpc_juce::gui
