#include "gui/ios/MobilePlatform.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#if JUCE_IOS
#import <UIKit/UIKit.h>
#import <objc/runtime.h>

namespace
{
    char statusBarHiddenKey;

    BOOL vmpcPrefersStatusBarHidden(id self, SEL)
    {
        NSNumber *value = objc_getAssociatedObject(self, &statusBarHiddenKey);
        return value != nil && value.boolValue;
    }

    UIViewController *rootController()
    {
        UIApplication *application = UIApplication.sharedApplication;
        if (@available(iOS 13.0, *))
        {
            for (UIScene *scene in application.connectedScenes)
            {
                if (![scene isKindOfClass:UIWindowScene.class])
                {
                    continue;
                }
                UIWindowScene *windowScene = (UIWindowScene *)scene;
                for (UIWindow *window in windowScene.windows)
                {
                    if (window.isKeyWindow && window.rootViewController != nil)
                    {
                        return window.rootViewController;
                    }
                }
            }
        }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        return application.keyWindow.rootViewController;
#pragma clang diagnostic pop
    }

    void installStatusBarOverride(UIViewController *controller)
    {
        Class currentClass = object_getClass(controller);
        NSString *currentName = NSStringFromClass(currentClass);
        if ([currentName hasSuffix:@"_VMPCStatusBar"])
        {
            return;
        }
        NSString *subclassName =
            [currentName stringByAppendingString:@"_VMPCStatusBar"];
        Class subclass = NSClassFromString(subclassName);
        if (subclass == Nil)
        {
            subclass = objc_allocateClassPair(currentClass,
                                              subclassName.UTF8String, 0);
            class_addMethod(subclass, @selector(prefersStatusBarHidden),
                            (IMP)vmpcPrefersStatusBarHidden, "c@:");
            objc_registerClassPair(subclass);
        }
        object_setClass(controller, subclass);
    }
} // namespace

bool vmpc_juce::gui::ios::isRunningOnIPhone()
{
    return UIDevice.currentDevice.userInterfaceIdiom ==
           UIUserInterfaceIdiomPhone;
}

void vmpc_juce::gui::ios::setIPhoneStatusBarHidden(const bool hidden)
{
    dispatch_async(dispatch_get_main_queue(), ^{
      UIViewController *controller = rootController();
      if (controller == nil)
      {
          return;
      }
      installStatusBarOverride(controller);
      objc_setAssociatedObject(controller, &statusBarHiddenKey, @(hidden),
                               OBJC_ASSOCIATION_RETAIN_NONATOMIC);
      [controller setNeedsStatusBarAppearanceUpdate];
    });
}

void vmpc_juce::gui::ios::setIPhoneOrientation(
    const arrangement::Orientation orientation)
{
    using Desktop = juce::Desktop;
    Desktop::getInstance().setOrientationsEnabled(
        orientation == arrangement::Orientation::portrait
            ? Desktop::upright
            : Desktop::rotatedClockwise | Desktop::rotatedAntiClockwise);
}
#endif
