#include "gui/mobile/MobilePlatform.hpp"

#if JUCE_IOS
#import <UIKit/UIKit.h>

bool vmpc_juce::gui::mobile::isPhone()
{
    return UIDevice.currentDevice.userInterfaceIdiom ==
           UIUserInterfaceIdiomPhone;
}
#endif
