#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#ifdef __APPLE__
#include "MacBundleResources.h"
#endif

namespace vmpc {
class ResourceUtil {

public:
    static juce::Image loadImage(const std::string &path);

private:
#ifdef __APPLE__
        static juce::Image loadImageFromMacBundleResources(const std::string &path);
#elif WIN32
        static juce::Image loadImageFromWindowsResourceLibrary(const std::string& path);
#else
        static juce::Image loadImageFromInMemoryFS(const std::string& path);
#endif
};
}
