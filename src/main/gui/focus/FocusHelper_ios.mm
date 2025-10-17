#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS

#import <UIKit/UIKit.h>

extern "C" bool isEditorKeyWindow(void* componentPeerNativeHandle)
{
    // On iOS, JUCE peers are typically UIView* or UIWindow*
    // We can safely check if that view’s window is the key window.

    if (componentPeerNativeHandle == nullptr)
        return false;

    UIView* view = (__bridge UIView*)componentPeerNativeHandle;
    UIWindow* window = view.window;

    if (window == nil)
        return false;

    // On iOS, only one window is "key" (the frontmost, active one)
    return [window isKeyWindow];
}

#endif
#endif
