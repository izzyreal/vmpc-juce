#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#ifdef __APPLE__
#define MAC_BUNDLE_RESOURCES 1
#endif

namespace vmpc_juce
{
    class VmpcJuceResourceUtil
    {

    public:
        static juce::Image loadImage(const std::string &path);
        static std::vector<char> getResourceData(const std::string &path);

    private:
#ifdef MAC_BUNDLE_RESOURCES
        static std::vector<char>
        getResourceDataFromMacBundleResources(const std::string &path);
        static juce::Image
        loadImageFromMacBundleResources(const std::string &path);
#else
        static std::vector<char>
        getResourceDataFromInMemoryFS(const std::string &path);
        static juce::Image loadImageFromInMemoryFS(const std::string &path);
#endif
    };
} // namespace vmpc_juce
