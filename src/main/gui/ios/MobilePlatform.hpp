#pragma once

#include "gui/arrangement/ArrangementModel.hpp"

#include <functional>

namespace vmpc_juce::gui::ios
{
    enum class AudioRecordingPermission
    {
        undetermined,
        denied,
        granted
    };

    bool isRunningOnIPhone();
    void setIPhoneStatusBarHidden(bool hidden);
    void setIPhoneOrientation(arrangement::Orientation orientation);
    AudioRecordingPermission getAudioRecordingPermission();
    void requestAudioRecordingPermission(std::function<void(bool)> callback);
    void openApplicationSettings();
} // namespace vmpc_juce::gui::ios
