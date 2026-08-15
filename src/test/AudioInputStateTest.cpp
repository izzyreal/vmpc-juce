#include <catch2/catch_test_macros.hpp>

#include "standalone/AudioInputState.hpp"

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
