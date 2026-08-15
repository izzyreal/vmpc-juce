#pragma once

#include <juce_core/juce_core.h>

namespace vmpc_juce::standalone
{
    inline bool
    isAudioInputExplicitlyDisabled(const juce::XmlElement *audioSetup)
    {
        if (audioSetup == nullptr || !audioSetup->hasTagName("DEVICESETUP") ||
            !audioSetup->hasAttribute("audioDeviceInChans"))
        {
            return false;
        }

        juce::BigInteger inputChannels;
        inputChannels.parseString(
            audioSetup->getStringAttribute("audioDeviceInChans"), 2);
        return inputChannels.isZero();
    }

    inline void forceAudioInputDisabled(juce::XmlElement &audioSetup)
    {
        audioSetup.setAttribute("audioDeviceInChans", "0");
    }
} // namespace vmpc_juce::standalone
