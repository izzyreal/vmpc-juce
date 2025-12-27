#pragma once

namespace vmpc_juce::standalone
{
    class AudioDeviceManager;

    struct AudioDeviceSetupDetails
    {
        AudioDeviceManager *manager;
        int minNumInputChannels, maxNumInputChannels;
        int minNumOutputChannels, maxNumOutputChannels;
    };
} // namespace vmpc_juce::standalone