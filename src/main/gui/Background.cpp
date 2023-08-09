#include "Background.h"
#include "../ResourceUtil.h"
#include "mpc_fs.hpp"
#include "Paths.hpp"

Background::Background()
{
  const auto skinPath = mpc::Paths::appDocumentsPath() / "Skin" / "bg.jpg";
  const bool skinExists = fs::exists(skinPath);

  if (skinExists)
  {
      const auto skinData = get_file_data(skinPath);
      img = juce::ImageFileFormat::loadFrom(&skinData[0], skinData.size());
  }
  else
  {
      img = ResourceUtil::loadImage("img/bg.jpg");
  }

  setBufferedToImage(true);
}

void Background::paint(juce::Graphics& g)
{
  g.drawImageWithin(img, 0, 0, getParentWidth(), getParentHeight(), juce::RectanglePlacement::centred);
}
