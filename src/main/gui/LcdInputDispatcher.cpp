#include "gui/LcdInteraction.hpp"
#include "gui/MouseWheelInput.hpp"

#include <Mpc.hpp>

namespace vmpc_juce::gui
{
    mpc::input::HostInputResult
    dispatchLcdPointerEvent(mpc::Mpc &mpc, const juce::MouseEvent &event,
                            const juce::Rectangle<float> renderedLcdBounds,
                            const mpc::input::GestureEvent::Type type)
    {
        const auto hostInput = makeLcdHostInputEvent(makeLcdPointerEvent(event),
                                                     renderedLcdBounds, type);
        return hostInput ? mpc.dispatchHostInput(*hostInput)
                         : mpc::input::HostInputResult::Ignored;
    }

    mpc::input::HostInputResult
    dispatchLcdWheelEvent(mpc::Mpc &mpc, const juce::MouseEvent &event,
                          const juce::MouseWheelDetails &wheel,
                          const juce::Rectangle<float> renderedLcdBounds)
    {
        const auto hostInput = makeLcdWheelHostInputEvent(
            makeLcdPointerEvent(event), renderedLcdBounds,
            mouseWheelContinuousDelta(wheel));
        return hostInput ? mpc.dispatchHostInput(*hostInput)
                         : mpc::input::HostInputResult::Ignored;
    }
} // namespace vmpc_juce::gui
