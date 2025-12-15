#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "standalone/DeviceSelectorComponent.hpp"

namespace vmpc_juce::standalone
{
    class AudioDeviceManager;

    class AudioMidiSettingsComponent final : public juce::Component
    {
    public:
        AudioMidiSettingsComponent(AudioDeviceManager &deviceManagerToUse,
                                   int maxAudioInputChannels,
                                   int maxAudioOutputChannels);

        void paint(juce::Graphics &g) override;

        void resized() override;

        void childBoundsChanged(Component *childComp) override
        {
            if (!isResizing && childComp == &deviceSelector)
            {
                setToRecommendedSize();
            }
        }

        void setToRecommendedSize();

    private:
        DeviceSelectorComponent deviceSelector;
        bool isResizing = false;
    };
} // namespace vmpc_juce::standalone
