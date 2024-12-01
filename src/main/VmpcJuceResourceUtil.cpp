#include "VmpcJuceResourceUtil.hpp"

#ifdef MAC_BUNDLE_RESOURCES
#include "MacBundleResources.h"
#else
#include <cmrc/cmrc.hpp>
#include <string_view>
CMRC_DECLARE(vmpcjuce);
#endif

using namespace vmpc_juce;

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
#else
juce::Image VmpceJuceResourceUtil::loadImageFromInMemoryFS(const std::string& path)
{
  auto fs = cmrc::vmpcjuce::get_filesystem();
  auto file = fs.open(path.c_str());
  auto data = std::string_view(file.begin(), file.size()).data();
  auto stream = juce::MemoryInputStream(data, file.size(), true);
  return juce::ImageFileFormat::loadFrom(stream);
}
#endif
