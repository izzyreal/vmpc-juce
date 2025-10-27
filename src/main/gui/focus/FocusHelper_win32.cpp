#ifdef _WIN32
#include <windows.hpp>

extern "C" bool isEditorKeyWindow(void *componentPeerNativeHandle)
{
    HWND hwnd = static_cast<HWND>(componentPeerNativeHandle);
    if (!hwnd)
    {
        return false;
    }

    HWND fg = GetForegroundWindow();
    return fg && fg == hwnd;
}
#endif
