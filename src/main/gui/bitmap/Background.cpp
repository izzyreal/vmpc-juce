/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
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
