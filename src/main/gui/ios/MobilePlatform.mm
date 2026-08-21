#include "gui/ios/MobilePlatform.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#if JUCE_IOS
#import <AVFAudio/AVFAudio.h>
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

std::string vmpc_juce::gui::ios::getAudioInputRouteDisplayName()
{
    const auto session = AVAudioSession.sharedInstance;

    if (const auto input = session.currentRoute.inputs.firstObject)
    {
        if (input.portName.length > 0)
        {
            return input.portName.UTF8String;
        }
    }

    for (AVAudioSessionPortDescription *output in session.currentRoute.outputs)
    {
        const auto isBuiltInOutput =
            [output.portType
                isEqualToString:AVAudioSessionPortBuiltInSpeaker] ||
            [output.portType isEqualToString:AVAudioSessionPortBuiltInReceiver];
        if (isBuiltInOutput)
        {
            const auto name = [UIDevice.currentDevice.localizedModel
                stringByAppendingString:@" Microphone"];
            return name.UTF8String;
        }
    }

    return {};
}

vmpc_juce::gui::ios::AudioRecordingPermission
vmpc_juce::gui::ios::getAudioRecordingPermission()
{
    if (@available(iOS 17.0, *))
    {
        switch (AVAudioApplication.sharedInstance.recordPermission)
        {
            case AVAudioApplicationRecordPermissionGranted:
                return AudioRecordingPermission::granted;
            case AVAudioApplicationRecordPermissionDenied:
                return AudioRecordingPermission::denied;
            case AVAudioApplicationRecordPermissionUndetermined:
                return AudioRecordingPermission::undetermined;
        }
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    switch (AVAudioSession.sharedInstance.recordPermission)
    {
        case AVAudioSessionRecordPermissionGranted:
            return AudioRecordingPermission::granted;
        case AVAudioSessionRecordPermissionDenied:
            return AudioRecordingPermission::denied;
        case AVAudioSessionRecordPermissionUndetermined:
            return AudioRecordingPermission::undetermined;
    }
#pragma clang diagnostic pop

    return AudioRecordingPermission::undetermined;
}

void vmpc_juce::gui::ios::requestAudioRecordingPermission(
    std::function<void(bool)> callback)
{
    if (!callback)
    {
        return;
    }

    const auto completion = ^(BOOL granted) {
      juce::MessageManager::callAsync(
          [callback, granted]
          {
              callback(granted == YES);
          });
    };

    if (@available(iOS 17.0, *))
    {
        [AVAudioApplication
            requestRecordPermissionWithCompletionHandler:completion];
        return;
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [AVAudioSession.sharedInstance requestRecordPermission:completion];
#pragma clang diagnostic pop
}

void vmpc_juce::gui::ios::openApplicationSettings()
{
    NSURL *url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
    if (url == nil)
    {
        return;
    }

    [UIApplication.sharedApplication openURL:url
                                     options:@{}
                           completionHandler:nil];
}
#endif
