#include "gui/ios/MobilePlatform.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#if !JUCE_IOS
bool vmpc_juce::gui::ios::isRunningOnIPhone()
{
    return false;
}

void vmpc_juce::gui::ios::setIPhoneStatusBarHidden(bool) {}

void vmpc_juce::gui::ios::setIPhoneOrientation(arrangement::Orientation) {}
#endif
