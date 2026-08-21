#include <catch2/catch_test_macros.hpp>

#include "gui/mobile/MobilePlatform.hpp"

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
