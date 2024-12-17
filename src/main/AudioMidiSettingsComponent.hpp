/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace vmpc_juce {
    class AudioMidiSettingsComponent : public juce::Component
    {
        public:
            AudioMidiSettingsComponent (juce::AudioDeviceManager& deviceManagerToUse,
                    int maxAudioInputChannels,
                    int maxAudioOutputChannels);

            void paint (juce::Graphics& g) override;

            void resized() override;

            void childBoundsChanged (Component* childComp) override;

            void setToRecommendedSize();

        private:
            juce::AudioDeviceSelectorComponent deviceSelector;
            bool isResizing = false;
    };
} // namespace vmpc_juce
