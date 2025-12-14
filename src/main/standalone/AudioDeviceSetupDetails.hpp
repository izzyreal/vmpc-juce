#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

namespace vmpc_juce::standalone
{
    struct AudioDeviceSetupDetails
    {
        juce::AudioDeviceManager *manager;
        int minNumInputChannels, maxNumInputChannels;
        int minNumOutputChannels, maxNumOutputChannels;
        bool useStereoPairs;
    };
}