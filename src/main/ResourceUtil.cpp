#include "ResourceUtil.h"

#ifdef __APPLE__
#include "MacBundleResources.h"
#elif WIN32
#include "Windows.h"
#else
#include <cmrc/cmrc.hpp>
#include <string_view>
CMRC_DECLARE(vmpcjuce);
#endif

using namespace vmpc;

juce::Image ResourceUtil::loadImage(const std::string& path)
{
#ifdef __APPLE__
    return loadImageFromMacBundleResources(path);
#elif WIN32
    return loadImageFromWindowsResourceLibrary(path);
#else
    return loadImageFromInMemoryFS(path);
#endif
}

#ifdef __APPLE__
juce::Image ResourceUtil::loadImageFromMacBundleResources(const std::string &path)
{
    const auto imgPath = mpc::MacBundleResources::getResourcePath(path);
    return juce::ImageFileFormat::loadFrom(juce::File(imgPath));
}
#elif WIN32
juce::Image ResourceUtil::loadImageFromWindowsResourceLibrary(const std::string& path)
{
    HMODULE hModule = LoadLibrary(TEXT("vmpc_juce_standalone_resources.dll"));
    HRSRC hRes = FindResource(hModule, path.c_str(), RT_RCDATA);
    HGLOBAL hData = LoadResource(hModule, hRes);
    void* pData = LockResource(hData);
    DWORD dataSize = SizeofResource(hModule, hRes);
    juce::MemoryInputStream inputStream(pData, dataSize, false);
    juce::Image image = juce::ImageFileFormat::loadFrom(inputStream);
    FreeLibrary(hModule);
    return image;

}
#else
juce::Image ResourceUtil::loadImageFromInMemoryFS(const std::string& path)
{
  auto fs = cmrc::vmpcjuce::get_filesystem();
  auto file = fs.open(path.c_str());
  auto data = std::string_view(file.begin(), file.size()).data();
  auto stream = juce::MemoryInputStream(data, file.size(), true);
  return juce::ImageFileFormat::loadFrom(stream);
}
#endif