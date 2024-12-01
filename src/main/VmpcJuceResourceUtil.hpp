#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#ifdef __APPLE__
#define MAC_BUNDLE_RESOURCES 1
#endif

namespace vmpc_juce {
    class VmpcJuceResourceUtil {

        public:
            static juce::Image loadImage(const std::string &path);

        private:
#ifdef MAC_BUNDLE_RESOURCES

            static juce::Image loadImageFromMacBundleResources(const std::string &path);

#else
            static juce::Image loadImageFromInMemoryFS(const std::string& path);
#endif
    };
}
