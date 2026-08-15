#include <catch2/catch_test_macros.hpp>

#include "gui/vector/MenuGeometry.hpp"

using namespace vmpc_juce::gui::vector;

TEST_CASE("Collapsed menu only intercepts its visible control",
          "[vmpc][menu][hit-test]")
{
    constexpr auto componentHeight = 32.f;
    const juce::Rectangle<int> hamburgerBounds{270, 6, 18, 18};
    const auto interactiveBounds = menu_geometry::getInteractiveBounds(
        hamburgerBounds, componentHeight, 1.f, false);

    CHECK(interactiveBounds.contains(279.f, 15.f));
    CHECK_FALSE(interactiveBounds.contains(40.f, 15.f));
}

TEST_CASE("Expanded menu intercepts the complete visible menu background",
          "[vmpc][menu][hit-test]")
{
    constexpr auto componentHeight = 32.f;
    const juce::Rectangle<int> visibleIconBounds{30, 6, 258, 18};
    const auto interactiveBounds = menu_geometry::getInteractiveBounds(
        visibleIconBounds, componentHeight, 1.f, true);

    CHECK(interactiveBounds.contains(40.f, 15.f));
    CHECK(interactiveBounds.contains(279.f, 15.f));
    CHECK_FALSE(interactiveBounds.contains(5.f, 15.f));
}
