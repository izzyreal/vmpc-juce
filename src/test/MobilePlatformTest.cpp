#include <catch2/catch_test_macros.hpp>

#include "gui/mobile/MobilePlatform.hpp"
#include "gui/mobile/SafeArea.hpp"

using vmpc_juce::gui::mobile::getSafeEditorScreenBounds;
using vmpc_juce::gui::mobile::isPhoneSizedDisplay;

TEST_CASE("Mobile form factor uses the shortest display side",
          "[vmpc][mobile][form-factor]")
{
    CHECK(isPhoneSizedDisplay(390, 844));
    CHECK(isPhoneSizedDisplay(844, 390));
    CHECK_FALSE(isPhoneSizedDisplay(600, 960));
    CHECK_FALSE(isPhoneSizedDisplay(960, 600));
    CHECK_FALSE(isPhoneSizedDisplay(800, 1280));
}

TEST_CASE("Invalid display dimensions are not classified as phones",
          "[vmpc][mobile][form-factor]")
{
    CHECK_FALSE(isPhoneSizedDisplay(0, 844));
    CHECK_FALSE(isPhoneSizedDisplay(390, 0));
    CHECK_FALSE(isPhoneSizedDisplay(-1, 844));
}

TEST_CASE("Mobile content avoids portrait cutouts and the home indicator",
          "[vmpc][mobile][safe-area]")
{
    const juce::Rectangle<int> display{0, 0, 390, 844};
    CHECK(getSafeEditorScreenBounds(display, display, {59, 0, 34, 0}) ==
          juce::Rectangle<int>{0, 59, 390, 751});
    CHECK(getSafeEditorScreenBounds(display, display, {}) == display);
}

TEST_CASE("Mobile safe bounds follow both landscape orientations",
          "[vmpc][mobile][safe-area]")
{
    const juce::Rectangle<int> display{0, 0, 844, 390};
    CHECK(getSafeEditorScreenBounds(display, display, {0, 44, 21, 0}) ==
          juce::Rectangle<int>{44, 0, 800, 369});
    CHECK(getSafeEditorScreenBounds(display, display, {0, 0, 21, 44}) ==
          juce::Rectangle<int>{0, 0, 800, 369});
}

TEST_CASE("Mobile hosts do not receive duplicate safe area padding",
          "[vmpc][mobile][safe-area]")
{
    const juce::Rectangle<int> display{0, 0, 390, 844};
    const juce::BorderSize<int> insets{59, 0, 34, 0};
    const juce::Rectangle<int> hostedEditor{10, 100, 370, 600};
    CHECK(getSafeEditorScreenBounds(hostedEditor, display, insets) ==
          hostedEditor);
    CHECK(getSafeEditorScreenBounds({0, 30, 390, 800}, display, insets) ==
          juce::Rectangle<int>{0, 59, 390, 751});
}

TEST_CASE("Mobile safe bounds account for display origins and window resizing",
          "[vmpc][mobile][safe-area]")
{
    const juce::Rectangle<int> display{100, 200, 390, 844};
    const juce::BorderSize<int> insets{59, 0, 34, 0};
    CHECK(getSafeEditorScreenBounds(display, display, insets) ==
          juce::Rectangle<int>{100, 259, 390, 751});
    CHECK(getSafeEditorScreenBounds({100, 200, 390, 400}, display, insets) ==
          juce::Rectangle<int>{100, 259, 390, 341});
    CHECK(getSafeEditorScreenBounds({}, display, insets).isEmpty());
    CHECK(getSafeEditorScreenBounds({100, 200, 390, 40}, display, insets)
              .isEmpty());
}
