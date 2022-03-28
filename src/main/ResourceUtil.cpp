#include "ResourceUtil.h"

#include <cmrc/cmrc.hpp>
#include <string_view>

CMRC_DECLARE(vmpcjuce);

juce::Image ResourceUtil::loadImage(std::string path)
{
  auto fs = cmrc::vmpcjuce::get_filesystem();
  auto file = fs.open(path.c_str());
  auto data = std::string_view(file.begin(), file.size()).data();
  auto stream = juce::MemoryInputStream(data, file.size(), true);
  return juce::ImageFileFormat::loadFrom(stream);
}
