#pragma once

#include <cstdint>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#endif

#ifdef __APPLE__
uint32_t getInitialWindowHeightApple();
#endif

namespace vmpc_juce {

    class InitialWindowDimensions {
        public:
            static std::pair<int, int> get(const int baseLayoutWidth, const int baseLayoutHeight)
            {
                int w, h;

                if (juce::JUCEApplication::isStandaloneApp())
                {
                    h = getInitialWindowHeight();

                    if (h == 0)
                    {
                        w = baseLayoutWidth; h = baseLayoutHeight;
                    }
                    else
                    {
                        w = h * (baseLayoutWidth / (float) baseLayoutHeight);
                    }
                }
                else
                {
                    w = baseLayoutWidth; h = baseLayoutHeight;
                }

                return { w, h };
            }

        private:
            static uint32_t getInitialWindowHeight()
            {
#ifdef __APPLE__
                return getInitialWindowHeightApple();
#elif defined(_WIN32) || defined(_WIN64)
                RECT workArea;
                SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

                HDC screen = GetDC(nullptr);
                int dpi = GetDeviceCaps(screen, LOGPIXELSY);
                ReleaseDC(nullptr, screen);

                const double scaleFactor = dpi / 96.0;
                const int totalHeight = workArea.bottom - workArea.top;
                const int frameHeight = static_cast<int>((GetSystemMetrics(SM_CYCAPTION) + 2 * GetSystemMetrics(SM_CYFRAME)) * scaleFactor);
                return static_cast<int>((totalHeight - frameHeight - 22) / scaleFactor);
#elif defined(__linux__)
                Display* display = XOpenDisplay(nullptr);

                if (!display)
                {
                    return 0;
                }

                int32_t height = DisplayHeight(display, DefaultScreen(display));
                XCloseDisplay(display);
                return height;
#else
                return 0;
#endif
            }
    };
} // namespace vmpc_juce
