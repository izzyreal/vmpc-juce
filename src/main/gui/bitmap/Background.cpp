#include "Background.hpp"
#include "mpc_fs.hpp"
#include "Mpc.hpp"

#include "VmpcJuceResourceUtil.hpp"

using namespace vmpc_juce::gui::bitmap;

Background::Background(mpc::Mpc& mpc)
{
  const auto skinPath = mpc.paths->appDocumentsPath() / "Skin" / "bg.jpg";
  const bool skinExists = fs::exists(skinPath);

  if (skinExists)
  {
      const auto skinData = get_file_data(skinPath);
      img = juce::ImageFileFormat::loadFrom(&skinData[0], skinData.size());
  }
  else
  {
      img = VmpcJuceResourceUtil::loadImage("img/bg.jpg");
  }

  setBufferedToImage(true);
}

void Background::paint(juce::Graphics& g)
{
  g.drawImageWithin(img, 0, 0, getParentWidth(), getParentHeight(), juce::RectanglePlacement::centred);
}