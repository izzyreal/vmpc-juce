#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace vmpc_juce
{
    class AudioMidiSettingsComponent : public juce::Component
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
        juce::AudioDeviceSelectorComponent deviceSelector;
        bool isResizing = false;
    };
} // namespace vmpc_juce
