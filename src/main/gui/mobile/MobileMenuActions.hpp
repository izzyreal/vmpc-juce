#pragma once

#include <functional>

namespace vmpc_juce::gui::mobile
{
    struct MobileMenuActions
    {
        std::function<void()> importFiles;
        std::function<void()> exportFiles;
        std::function<void()> openRecordingManager;
        std::function<void()> togglePhoneFullscreen;
    };
} // namespace vmpc_juce::gui::mobile
