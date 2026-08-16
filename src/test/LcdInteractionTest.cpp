#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "gui/LcdInteraction.hpp"
#include "gui/MouseWheelInput.hpp"

#include <hardware/ComponentId.hpp>

#include <limits>

using namespace vmpc_juce::gui;
using mpc::hardware::ComponentId;
using mpc::input::GestureEvent;
using mpc::input::HostInputEvent;

namespace
{
    LcdPointerEvent pointer(const float x, const float y,
                            const int sourceIndex = 0,
                            const GestureEvent::InputDeviceType deviceType =
                                GestureEvent::InputDeviceType::Mouse)
    {
        return {{x, y}, sourceIndex, deviceType, true};
    }

    const GestureEvent &gesture(const HostInputEvent &event)
    {
        return std::get<GestureEvent>(event.payload);
    }
} // namespace

TEST_CASE("LCD pointer mapping produces normalized component gestures",
          "[vmpc][lcd][input]")
{
    const juce::Rectangle<float> bounds{10.f, 20.f, 496.f, 120.f};
    auto source = pointer(258.f, 80.f, 7, GestureEvent::InputDeviceType::Touch);
    source.shiftDown = true;
    source.altDown = true;

    const auto event =
        makeLcdHostInputEvent(source, bounds, GestureEvent::Type::BEGIN);
    REQUIRE(event);
    const auto &mapped = gesture(*event);
    CHECK(mapped.type == GestureEvent::Type::BEGIN);
    CHECK(mapped.movement == GestureEvent::Movement::Absolute);
    CHECK(mapped.normX == Catch::Approx(0.5f));
    CHECK(mapped.normY == Catch::Approx(0.5f));
    CHECK(mapped.sourceIndex == 7);
    CHECK(mapped.componentId == ComponentId::LCD);
    CHECK(mapped.inputDeviceType == GestureEvent::InputDeviceType::Touch);
    CHECK(mapped.shiftDown);
    CHECK_FALSE(mapped.ctrlDown);
    CHECK(mapped.altDown);
}

TEST_CASE("LCD begins require primary contact inside rendered bounds",
          "[vmpc][lcd][input]")
{
    const juce::Rectangle<float> bounds{10.f, 20.f, 248.f, 60.f};
    CHECK_FALSE(makeLcdHostInputEvent(pointer(9.f, 50.f), bounds,
                                      GestureEvent::Type::BEGIN));
    CHECK_FALSE(makeLcdHostInputEvent(pointer(258.f, 50.f), bounds,
                                      GestureEvent::Type::BEGIN));

    auto secondary = pointer(100.f, 50.f);
    secondary.isPrimary = false;
    CHECK_FALSE(
        makeLcdHostInputEvent(secondary, bounds, GestureEvent::Type::BEGIN));
    CHECK_FALSE(makeLcdHostInputEvent(pointer(100.f, 50.f), {},
                                      GestureEvent::Type::BEGIN));
}

TEST_CASE("Captured LCD updates and ends clamp outside positions",
          "[vmpc][lcd][input]")
{
    const juce::Rectangle<float> bounds{10.f, 20.f, 248.f, 60.f};
    const auto update = makeLcdHostInputEvent(pointer(-100.f, 200.f), bounds,
                                              GestureEvent::Type::UPDATE);
    REQUIRE(update);
    CHECK(gesture(*update).normX == Catch::Approx(0.f));
    CHECK(gesture(*update).normY == Catch::Approx(1.f));

    auto released = pointer(500.f, -50.f);
    released.isPrimary = false;
    const auto end =
        makeLcdHostInputEvent(released, bounds, GestureEvent::Type::END);
    REQUIRE(end);
    CHECK(gesture(*end).normX == Catch::Approx(1.f));
    CHECK(gesture(*end).normY == Catch::Approx(0.f));
}

TEST_CASE("LCD adapter rejects invalid positions and repeat events",
          "[vmpc][lcd][input]")
{
    const juce::Rectangle<float> bounds{0.f, 0.f, 248.f, 60.f};
    CHECK_FALSE(makeLcdHostInputEvent(
        pointer(std::numeric_limits<float>::quiet_NaN(), 20.f), bounds,
        GestureEvent::Type::UPDATE));
    CHECK_FALSE(makeLcdHostInputEvent(pointer(100.f, 20.f), bounds,
                                      GestureEvent::Type::REPEAT));
}

TEST_CASE("Mouse wheel delta conversion matches hardware controls",
          "[vmpc][lcd][input][wheel]")
{
    juce::MouseWheelDetails wheel{};
    wheel.deltaY = 0.25f;

    CHECK(mouseWheelContinuousDelta(wheel) == Catch::Approx(-2.5f));

    wheel.isSmooth = true;
    CHECK(mouseWheelContinuousDelta(wheel) == Catch::Approx(-10.f));

    wheel.isInertial = true;
    CHECK(mouseWheelContinuousDelta(wheel) == Catch::Approx(-20.f));

    wheel.isSmooth = false;
    CHECK(mouseWheelContinuousDelta(wheel) == Catch::Approx(-5.f));
}

TEST_CASE("LCD wheel mapping produces relative gestures inside the display",
          "[vmpc][lcd][input][wheel]")
{
    const juce::Rectangle<float> bounds{10.f, 20.f, 248.f, 60.f};
    auto source = pointer(134.f, 50.f, 8);
    source.ctrlDown = true;

    const auto event = makeLcdWheelHostInputEvent(source, bounds, -12.5f);
    REQUIRE(event);
    const auto &mapped = gesture(*event);
    CHECK(mapped.type == GestureEvent::Type::UPDATE);
    CHECK(mapped.movement == GestureEvent::Movement::Relative);
    CHECK(mapped.normX == Catch::Approx(0.5f));
    CHECK(mapped.normY == Catch::Approx(0.5f));
    CHECK(mapped.continuousDelta == Catch::Approx(-12.5f));
    CHECK(mapped.sourceIndex == 8);
    CHECK(mapped.componentId == ComponentId::LCD);
    CHECK(mapped.ctrlDown);

    CHECK_FALSE(makeLcdWheelHostInputEvent(pointer(9.f, 50.f), bounds, 1.f));
    CHECK_FALSE(makeLcdWheelHostInputEvent(pointer(100.f, 50.f), bounds, 0.f));
    CHECK_FALSE(makeLcdWheelHostInputEvent(
        pointer(100.f, 50.f), bounds, std::numeric_limits<float>::quiet_NaN()));
    CHECK_FALSE(makeLcdWheelHostInputEvent(pointer(100.f, 50.f), {}, 1.f));
}
