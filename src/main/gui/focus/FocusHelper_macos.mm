#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX

#include "gui/focus/FocusHelper.hpp"

#include <Cocoa/Cocoa.h>

extern "C" bool isEditorKeyWindow(void* nsviewPtr)
{
    NSView* view = (__bridge NSView*)nsviewPtr;
    return [NSApp keyWindow] == [view window];
}

#endif
#endif
