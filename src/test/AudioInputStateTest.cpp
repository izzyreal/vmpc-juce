#include <catch2/catch_test_macros.hpp>

#include "standalone/DeviceSelectorComponentLookAndFeel.hpp"
#include "standalone/AudioInputState.hpp"
#include "standalone/Utils.hpp"

using namespace vmpc_juce::standalone;

TEST_CASE("Audio input defaults to enabled without an explicit zero mask",
          "[vmpc][audio-input]")
{
    CHECK_FALSE(isAudioInputExplicitlyDisabled(nullptr));

    juce::XmlElement wrongElement("OTHER");
    wrongElement.setAttribute("audioDeviceInChans", "0");
    CHECK_FALSE(isAudioInputExplicitlyDisabled(&wrongElement));

    juce::XmlElement defaultSetup("DEVICESETUP");
    CHECK_FALSE(isAudioInputExplicitlyDisabled(&defaultSetup));

    defaultSetup.setAttribute("audioDeviceInChans", "11");
    CHECK_FALSE(isAudioInputExplicitlyDisabled(&defaultSetup));
}

TEST_CASE("An explicit zero input mask disables audio input",
          "[vmpc][audio-input]")
{
    juce::XmlElement setup("DEVICESETUP");
    setup.setAttribute("audioDeviceInChans", "0");

    CHECK(isAudioInputExplicitlyDisabled(&setup));
}

TEST_CASE("Forcing input off preserves the rest of the audio setup",
          "[vmpc][audio-input]")
{
    juce::XmlElement setup("DEVICESETUP");
    setup.setAttribute("audioDeviceInChans", "11");
    setup.setAttribute("audioDeviceOutChans", "1100");
    setup.setAttribute("audioDeviceRate", 48000);

    forceAudioInputDisabled(setup);

    CHECK(isAudioInputExplicitlyDisabled(&setup));
    CHECK(setup.getStringAttribute("audioDeviceOutChans") == "1100");
    CHECK(setup.getIntAttribute("audioDeviceRate") == 48000);
}

TEST_CASE("A single record input pair uses the audio route name",
          "[vmpc][audio-input]")
{
    const auto names = Utils::getRecordInputDisplayNames({"Left + Right"},
                                                         "iPhone Microphone");

    REQUIRE(names.size() == 1);
    CHECK(names[0] == "iPhone Microphone");
}

TEST_CASE("Multiple record input pairs include their channel names",
          "[vmpc][audio-input]")
{
    const auto names = Utils::getRecordInputDisplayNames(
        {"Input 1 + 2", "Input 3 + 4"}, "USB Interface");

    REQUIRE(names.size() == 2);
    CHECK(names[0] == "USB Interface — Input 1 + 2");
    CHECK(names[1] == "USB Interface — Input 3 + 4");
}

TEST_CASE("Record input pairs remain unchanged without a known audio route",
          "[vmpc][audio-input]")
{
    const juce::StringArray channelPairs{"Left + Right"};
    const auto names =
        Utils::getRecordInputDisplayNames(channelPairs, juce::String{});

    CHECK(names == channelPairs);
}

TEST_CASE("Audio settings popup menus draw a border",
          "[vmpc][audio-settings][look-and-feel]")
{
    constexpr int width = 12;
    constexpr int height = 10;

    const juce::ScopedJuceInitialiser_GUI juceInitialiser;
    const juce::Font font(juce::Font::getDefaultSansSerifFontName(), 18.0f,
                          juce::Font::plain);
    DeviceSelectorComponentLookAndFeel lookAndFeel(font);
    juce::Image image(juce::Image::ARGB, width, height, true);
    juce::Graphics graphics(image);

    lookAndFeel.drawPopupMenuBackground(graphics, width, height);

    CHECK(image.getPixelAt(width / 2, 0) == juce::Colours::black);
    CHECK(image.getPixelAt(width / 2, height - 1) == juce::Colours::black);
    CHECK(image.getPixelAt(0, height / 2) == juce::Colours::black);
    CHECK(image.getPixelAt(width - 1, height / 2) == juce::Colours::black);
    CHECK(image.getPixelAt(width / 2, height / 2) == juce::Colours::slategrey);
}
