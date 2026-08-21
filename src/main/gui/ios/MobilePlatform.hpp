#pragma once

#include <functional>
#include <string>

namespace vmpc_juce::gui::ios
{
    enum class AudioRecordingPermission
    {
        undetermined,
        denied,
        granted
    };

    void setIPhoneStatusBarHidden(bool hidden);
    std::string getAudioInputRouteDisplayName();
    AudioRecordingPermission getAudioRecordingPermission();
    void requestAudioRecordingPermission(std::function<void(bool)> callback);
    void openApplicationSettings();
} // namespace vmpc_juce::gui::ios
