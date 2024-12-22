#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include <UIKit/UIKit.h>
#else
#include <Cocoa/Cocoa.h>
#endif

uint32_t getInitialWindowHeightApple()
{
#if TARGET_OS_IPHONE
    UIScreen *screen = [UIScreen mainScreen];
    CGFloat screenHeight = screen.bounds.size.height;
    return static_cast<uint32_t>(screenHeight);
#elif TARGET_OS_MAC
    NSRect screenFrame = [[NSScreen mainScreen] visibleFrame];
    CGFloat titleBarHeight = [NSWindow frameRectForContentRect:NSMakeRect(0, 0, 0, 0)
                                                styleMask:NSWindowStyleMaskTitled].size.height;
    return static_cast<uint32_t>(screenFrame.size.height - titleBarHeight);
#else
    #error Unsupported platform
#endif
}
#endif
