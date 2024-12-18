#include <Cocoa/Cocoa.h>

uint32_t getInitialWindowHeightApple()
{
    NSRect screenFrame = [[NSScreen mainScreen] visibleFrame];
    CGFloat titleBarHeight = [NSWindow frameRectForContentRect:NSMakeRect(0, 0, 0, 0)
                                                styleMask:NSWindowStyleMaskTitled].size.height;
    return static_cast<uint32_t>(screenFrame.size.height - titleBarHeight);
}
