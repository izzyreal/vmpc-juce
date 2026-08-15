#include "gui/ios/MobilePlatform.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#if !JUCE_IOS
bool vmpc_juce::gui::ios::isRunningOnIPhone()
{
    return false;
}

void vmpc_juce::gui::ios::setIPhoneStatusBarHidden(bool) {}

void vmpc_juce::gui::ios::setIPhoneOrientation(arrangement::Orientation) {}

vmpc_juce::gui::ios::AudioRecordingPermission
vmpc_juce::gui::ios::getAudioRecordingPermission()
{
    return AudioRecordingPermission::granted;
}

void vmpc_juce::gui::ios::requestAudioRecordingPermission(
    std::function<void(bool)> callback)
{
    if (callback)
    {
        callback(true);
    }
}

void vmpc_juce::gui::ios::openApplicationSettings() {}
#endif
