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
#pragma once

#include "SvgComponent.hpp"

#include <Observer.hpp>

#include <memory>

namespace mpc { class Mpc; }

namespace mpc::hardware {
    class HwPad;
}

namespace vmpc_juce::gui::vector {

class Pad
        : public SvgComponent,
          public juce::Timer,
          public juce::FileDragAndDropTarget,
          public mpc::Observer
{

private:
    mpc::Mpc &mpc;
    std::weak_ptr<mpc::hardware::HwPad> pad;
    juce::Rectangle<int> rect;
    SvgComponent *glowSvg = nullptr;

    bool fading = false;

    int getVelo(int veloY);
    void loadFile(const juce::String path, bool shouldBeConverted, std::string screenToReturnTo);

public:
    void resized() override;
    void paint(juce::Graphics &g) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void timerCallback() override;
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;

    void update(mpc::Observable *o, mpc::Message message) override;

    Pad(juce::Component *commonParentWithShadowToUse, const float shadowSizeToUse, const std::function<float()> &getScaleToUse, mpc::Mpc &, std::weak_ptr<mpc::hardware::HwPad>);
    ~Pad() override;
};

} // namespace vmpc_juce::gui::vector
