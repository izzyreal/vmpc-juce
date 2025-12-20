#ifdef _WIN32
#include <windows.h>

extern "C" bool isEditorKeyWindow(void *componentPeerNativeHandle)
{
    HWND hwnd = static_cast<HWND>(componentPeerNativeHandle);
    if (!hwnd)
    {
        return false;
    }

    // This also works (only tested with Renoise), but I'm not sure which one
    // is better. GetFocus() seems to be the 'correct' one, because we're
    // looking for keyboard focus. But we may need to dig into the details after
    // trying against a few different hosts.
    //
    // HWND fg = GetForegroundWindow();

    HWND fg = GetFocus();
    return fg && (fg == hwnd || IsChild(fg, hwnd));
}
#endif
