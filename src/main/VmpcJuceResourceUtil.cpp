#include "VmpcJuceResourceUtil.hpp"

#ifdef MAC_BUNDLE_RESOURCES
#include "MacBundleResources.hpp"
#include "mpc_fs.hpp"
#else
#include <cmrc/cmrc.hpp>
#include <string_view>
CMRC_DECLARE(vmpcjuce);
#endif

using namespace vmpc_juce;

std::vector<char> VmpcJuceResourceUtil::getResourceData(const std::string& path)
{
#ifdef MAC_BUNDLE_RESOURCES
    return getResourceDataFromMacBundleResources(path);
#else
    return getResourceDataFromInMemoryFS(path);
#endif
}

juce::Image VmpcJuceResourceUtil::loadImage(const std::string& path)
{
#ifdef MAC_BUNDLE_RESOURCES
    return loadImageFromMacBundleResources(path);
#else
    return loadImageFromInMemoryFS(path);
#endif
}

#ifdef MAC_BUNDLE_RESOURCES
juce::Image VmpcJuceResourceUtil::loadImageFromMacBundleResources(const std::string &path)
{
    const auto imgPath = mpc::MacBundleResources::getResourcePath(path);
    return juce::ImageFileFormat::loadFrom(juce::File(imgPath));
}

std::vector<char> VmpcJuceResourceUtil::getResourceDataFromMacBundleResources(const std::string& path)
{
    const auto resource_path = mpc::MacBundleResources::getResourcePath(path);
    return get_file_data(resource_path);
}
#else

std::vector<char> VmpcJuceResourceUtil::getResourceDataFromInMemoryFS(const std::string& path)
{
    const auto file = cmrc::vmpcjuce::get_filesystem().open(path.c_str());
    const auto data = std::string_view(file.begin(), file.size()).data();
    return { data, data + file.size() };
}

juce::Image VmpcJuceResourceUtil::loadImageFromInMemoryFS(const std::string& path)
{
    const auto file = cmrc::vmpcjuce::get_filesystem().open(path.c_str());
    const auto data = std::string_view(file.begin(), file.size()).data();
    auto stream = juce::MemoryInputStream(data, file.size(), true);
    return juce::ImageFileFormat::loadFrom(stream);
}
#endif
