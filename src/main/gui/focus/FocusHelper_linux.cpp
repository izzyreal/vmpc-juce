#if defined(__ANDROID__)
extern "C" bool isEditorKeyWindow(void *componentPeerNativeHandle)
{
    // Android has a single JUCE activity window. The remaining FocusHelper
    // checks cover activity/process and component focus state.
    return componentPeerNativeHandle != nullptr;
}
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>

extern "C" bool isEditorKeyWindow(void *componentPeerNativeHandle)
{
    if (!componentPeerNativeHandle)
    {
        return false;
    }

    Display *display = XOpenDisplay(nullptr);
    if (!display)
    {
        return false;
    }

    ::Window window = reinterpret_cast<::Window>(componentPeerNativeHandle);
    ::Window focused;
    int revert;
    XGetInputFocus(display, &focused, &revert);

    bool result = (focused == window);
    XCloseDisplay(display);
    return result;
}
#endif
