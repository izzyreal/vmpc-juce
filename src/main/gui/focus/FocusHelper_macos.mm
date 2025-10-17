#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX

#include "FocusHelper.h"

#include <Cocoa/Cocoa.h>

extern "C" bool isEditorKeyWindowMacOs(void* nsviewPtr)
{
    NSView* view = (__bridge NSView*)nsviewPtr;
    return [NSApp keyWindow] == [view window];
}

#endif
#endif
