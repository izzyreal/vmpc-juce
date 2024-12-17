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
#include "VmpcComponent.hpp"

#include <Observer.hpp>

#include <memory>

namespace vmpc_juce::gui { class AuxLCDWindow; }

namespace mpc { class Mpc; }

namespace mpc::lcdgui {
	class LayeredScreen;
}

namespace vmpc_juce::gui::bitmap {
class LCDControl
	: public VmpcComponent
	, public juce::Timer
    , public mpc::Observer
{

private:
    vmpc_juce::gui::AuxLCDWindow* auxWindow = nullptr;
    mpc::Mpc& mpc;
	std::shared_ptr<mpc::lcdgui::LayeredScreen> ls;
	juce::Image lcd;
    juce::Rectangle<int> dirtyRect;
    std::function<void()> resetAuxWindowF;
    std::function<void()> resetKeyboardAuxParent;
    std::function<juce::Image&()> getLcdImage;

public:
    void resetAuxWindow();
    void checkLsDirty();
	void drawPixelsToImg();
	void paint(juce::Graphics& g) override;
	void timerCallback() override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
  void mouseDown(const juce::MouseEvent& e) override {
    getParentComponent()->mouseDown(e);
  }
  void mouseDrag(const juce::MouseEvent& e) override {
    getParentComponent()->mouseDrag(e);
  }

public:
    LCDControl(mpc::Mpc& mpc);
    ~LCDControl() override;

    void update(mpc::Observable* o, mpc::Message) override;
};
} // namespace vmpc_juce::gui::bitmap
