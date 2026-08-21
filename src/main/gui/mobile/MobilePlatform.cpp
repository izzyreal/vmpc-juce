#include "gui/mobile/MobilePlatform.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::mobile
{
    bool isMobilePlatform()
    {
#if JUCE_IOS || JUCE_ANDROID
        return true;
#else
        return false;
#endif
    }

#if !JUCE_IOS
    bool isPhone()
    {
#if JUCE_ANDROID
        if (const auto *display = juce::Desktop::getInstance()
                                      .getDisplays()
                                      .getPrimaryDisplay())
        {
            return isPhoneSizedDisplay(display->totalArea.getWidth(),
                                       display->totalArea.getHeight());
        }
#endif
        return false;
    }
#endif

    void setOrientation(const arrangement::Orientation orientation)
    {
#if JUCE_IOS || JUCE_ANDROID
        using Desktop = juce::Desktop;
        Desktop::getInstance().setOrientationsEnabled(
            orientation == arrangement::Orientation::portrait
                ? Desktop::upright
                : Desktop::rotatedClockwise | Desktop::rotatedAntiClockwise);
#else
        juce::ignoreUnused(orientation);
#endif
    }
} // namespace vmpc_juce::gui::mobile
