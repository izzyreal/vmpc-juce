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

#include "VmpcTooltipComponent.hpp"

#include <Observer.hpp>

#include <thread>
#include <memory>

namespace mpc { class Mpc; }

namespace mpc::hardware {
    class HwPad;
}

class PadControl
        : public VmpcTooltipComponent,
          public juce::FileDragAndDropTarget,
          public mpc::Observer
{

private:
    mpc::Mpc &mpc;
    std::weak_ptr<mpc::hardware::HwPad> pad;
    juce::Image padhitImg;
    juce::Rectangle<int> rect;

    bool fading = false;
    int padhitBrightness = 0;

    int getVelo(int veloX, int veloY);
    void loadFile(const juce::String path, bool shouldBeConverted, std::string screenToReturnTo);

public:
    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseDoubleClick(const juce::MouseEvent &event) override;
    void timerCallback() override;
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;

public:
    void update(mpc::Observable *o, mpc::Message message) override;
    void setBounds();

public:
    PadControl(mpc::Mpc &_mpc, juce::Rectangle<int> rectToUse, std::weak_ptr<mpc::hardware::HwPad> padToUse,
               juce::Image padHitImgToUse);
    ~PadControl() override;
};
