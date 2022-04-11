#include "Background.h"
#include "../ResourceUtil.h"

Background::Background()
{
  img = ResourceUtil::loadImage("img/bg.jpg");
  setBufferedToImage(true);
}

void Background::paint(juce::Graphics& g)
{
  g.drawImageWithin(img, 0, 0, getParentWidth(), getParentHeight(), juce::RectanglePlacement::centred);
}
