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

#include <string>
#include <memory>
#include <unordered_map>

namespace mpc ::hardware {
class Button;
}

namespace vmpc_juce::gui::bitmap {

class ButtonControl : public VmpcTooltipComponent
{
public:
    ButtonControl(mpc::Mpc& mpc, juce::Rectangle<int> _rect,
                  std::weak_ptr<mpc::hardware::Button> _button);

    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void setBounds();

    static std::unordered_map<std::string, juce::Rectangle<int>*> rects;
    static void initRects();

private:
    juce::Rectangle<int> rect;
    std::weak_ptr<mpc::hardware::Button> button;
    static const int smw = 30;
    static const int smh = 18;
    static const int mw = 40;
    static const int mh = 20;
    static const int bw = 48;
    static const int bh = 35;

    static juce::Rectangle<int> undoseq;
    static juce::Rectangle<int> erase;
    static juce::Rectangle<int> rec;
    static juce::Rectangle<int> overdub;
    static juce::Rectangle<int> stop;
    static juce::Rectangle<int> play;
    static juce::Rectangle<int> playstart;
    static juce::Rectangle<int> mainscreen;
    static juce::Rectangle<int> openwindow;
    static juce::Rectangle<int> taptemponoterepeat;
    static juce::Rectangle<int> prevstepevent;
    static juce::Rectangle<int> nextstepevent;
    static juce::Rectangle<int> gotoRect;
    static juce::Rectangle<int> prevbarstart;
    static juce::Rectangle<int> nextbarend;
    static juce::Rectangle<int> f1;
    static juce::Rectangle<int> f2;
    static juce::Rectangle<int> f3;
    static juce::Rectangle<int> f4;
    static juce::Rectangle<int> f5;
    static juce::Rectangle<int> f6;
    static juce::Rectangle<int> notevariationafter;
    static juce::Rectangle<int> rect0;
    static juce::Rectangle<int> rect1;
    static juce::Rectangle<int> rect2;
    static juce::Rectangle<int> rect3;
    static juce::Rectangle<int> rect4;
    static juce::Rectangle<int> rect5;
    static juce::Rectangle<int> rect6;
    static juce::Rectangle<int> rect7;
    static juce::Rectangle<int> rect8;
    static juce::Rectangle<int> rect9;
    static juce::Rectangle<int> shift;
    static juce::Rectangle<int> enter;
    static juce::Rectangle<int> banka;
    static juce::Rectangle<int> bankb;
    static juce::Rectangle<int> bankc;
    static juce::Rectangle<int> bankd;
    static juce::Rectangle<int> fulllevel;
    static juce::Rectangle<int> sixteenlevels;
    static juce::Rectangle<int> nextseq;
    static juce::Rectangle<int> trackmute;
    static juce::Rectangle<int> left;
    static juce::Rectangle<int> up;
    static juce::Rectangle<int> down;
    static juce::Rectangle<int> right;
};

} // namespace vmpc_juce::gui::bitmap
