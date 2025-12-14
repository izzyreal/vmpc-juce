#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace vmpc_juce::standalone
{
    struct InputLevelMeter final : juce::Component, juce::Timer
    {
        explicit InputLevelMeter(juce::AudioDeviceManager &m) : manager(m)
        {
            startTimerHz(20);
            inputLevelGetter = manager.getInputLevelGetter();
        }

        void timerCallback() override
        {
            if (isShowing())
            {
                const auto newLevel =
                    static_cast<float>(inputLevelGetter->getCurrentLevel());

                if (std::abs(level - newLevel) > 0.005f)
                {
                    level = newLevel;
                    repaint();
                }
            }
            else
            {
                level = 0;
            }
        }

        void paint(juce::Graphics &g) override
        {
            // (add a bit of a skew to make the level more obvious)
            getLookAndFeel().drawLevelMeter(
                g, getWidth(), getHeight(),
                static_cast<float>(std::exp(std::log(level) / 3.0)));
        }

        juce::AudioDeviceManager &manager;
        juce::AudioDeviceManager::LevelMeter::Ptr inputLevelGetter;
        float level = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputLevelMeter)
    };
} // namespace vmpc_juce::standalone