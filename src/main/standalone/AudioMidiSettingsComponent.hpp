#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "standalone/VmpcStandaloneDeviceSelectorComponent.hpp"

namespace vmpc_juce::standalone
{
    class AudioMidiSettingsComponent final : public juce::Component
    {
    public:
        AudioMidiSettingsComponent(juce::AudioDeviceManager &deviceManagerToUse,
                                   int maxAudioInputChannels,
                                   int maxAudioOutputChannels);

        void paint(juce::Graphics &g) override;

        void resized() override;

        void childBoundsChanged(Component *childComp) override;

        void setToRecommendedSize();

    private:
        VmpcStandaloneDeviceSelectorComponent deviceSelector;
        bool isResizing = false;
    };
} // namespace vmpc_juce
