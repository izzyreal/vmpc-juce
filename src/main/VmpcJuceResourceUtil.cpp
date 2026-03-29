#include "VmpcJuceResourceUtil.hpp"

#ifdef MAC_BUNDLE_RESOURCES
#include "MacBundleResources.hpp"
#include "Logger.hpp"
#include "mpc_fs.hpp"
#else
#include <cmrc/cmrc.hpp>
#include <string_view>
CMRC_DECLARE(vmpcjuce);
#endif

using namespace vmpc_juce;

std::string VmpcJuceResourceUtil::resolveResourcePath(const std::string &path)
{
#ifdef MAC_BUNDLE_RESOURCES
    return mpc::MacBundleResources::getResourcePath(path);
#else
    return path;
#endif
}

std::vector<char> VmpcJuceResourceUtil::getResourceData(const std::string &path)
{
#ifdef MAC_BUNDLE_RESOURCES
    return getResourceDataFromMacBundleResources(path);
#else
    return getResourceDataFromInMemoryFS(path);
#endif
}

bool VmpcJuceResourceUtil::resourceExists(const std::string &path)
{
#ifdef MAC_BUNDLE_RESOURCES
    const auto resolvedPath = resolveResourcePath(path);
    const auto existsRes = mpc_fs::exists(resolvedPath);
    if (!existsRes)
    {
        MLOG("Failed to inspect VMPC resource '" + path + "' at '" +
             resolvedPath + "': " + existsRes.error().message);
        return false;
    }

    return *existsRes;
#else
    try
    {
        (void) cmrc::vmpcjuce::get_filesystem().open(path.c_str());
        return true;
    }
    catch (...)
    {
        return false;
    }
#endif
}

juce::Image VmpcJuceResourceUtil::loadImage(const std::string &path)
{
#ifdef MAC_BUNDLE_RESOURCES
    return loadImageFromMacBundleResources(path);
#else
    return loadImageFromInMemoryFS(path);
#endif
}

#ifdef MAC_BUNDLE_RESOURCES
juce::Image
VmpcJuceResourceUtil::loadImageFromMacBundleResources(const std::string &path)
{
    const auto imgPath = mpc::MacBundleResources::getResourcePath(path);
    return juce::ImageFileFormat::loadFrom(juce::File(imgPath));
}

std::vector<char> VmpcJuceResourceUtil::getResourceDataFromMacBundleResources(
    const std::string &path)
{
    const auto resource_path = mpc::MacBundleResources::getResourcePath(path);
    const auto dataRes = get_file_data(resource_path);
    if (!dataRes)
    {
        MLOG("Failed to read VMPC resource '" + path + "' from '" +
             resource_path + "': " + dataRes.error().message);
        return {};
    }

    return *dataRes;
}
#else

std::vector<char>
VmpcJuceResourceUtil::getResourceDataFromInMemoryFS(const std::string &path)
{
    const auto file = cmrc::vmpcjuce::get_filesystem().open(path.c_str());
    const auto data = std::string_view(file.begin(), file.size()).data();
    return {data, data + file.size()};
}

juce::Image
VmpcJuceResourceUtil::loadImageFromInMemoryFS(const std::string &path)
{
    const auto file = cmrc::vmpcjuce::get_filesystem().open(path.c_str());
    const auto data = std::string_view(file.begin(), file.size()).data();
    auto stream = juce::MemoryInputStream(data, file.size(), true);
    return juce::ImageFileFormat::loadFrom(stream);
}
#endif
