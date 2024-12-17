/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
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
            static std::pair<int, int> get()
            {
                const int safeW = 445, safeH = 342;

                int w, h;

                if (juce::JUCEApplication::isStandaloneApp())
                {
                    h = getInitialWindowHeight();

                    if (h == 0)
                    {
                        w = safeW; h = safeH;
                    }
                    else
                    {
                        w = h * (safeW / (float) safeH);
                    }
                }
                else
                {
                    w = safeW; h = safeH;
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
