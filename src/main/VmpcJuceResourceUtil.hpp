#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <string>
#include <vector>

#ifdef __APPLE__
#define MAC_BUNDLE_RESOURCES 1
#endif

namespace vmpc_juce
{
    struct MissingRequiredResource
    {
        std::string logicalPath;
        std::string resolvedPath;
    };

    class VmpcJuceResourceUtil
    {

    public:
        static juce::Image loadImage(const std::string &path);
        static std::vector<char> getResourceData(const std::string &path);
        static bool resourceExists(const std::string &path);
        static std::string resolveResourcePath(const std::string &path);

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
