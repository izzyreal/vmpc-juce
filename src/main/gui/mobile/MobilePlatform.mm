#include "gui/mobile/MobilePlatform.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#if JUCE_IOS
#import <UIKit/UIKit.h>

bool vmpc_juce::gui::mobile::isPhone()
{
    return UIDevice.currentDevice.userInterfaceIdiom ==
           UIUserInterfaceIdiomPhone;
}
#endif
