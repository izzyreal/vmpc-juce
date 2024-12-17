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

#include <juce_gui_basics/juce_gui_basics.h>

namespace mpc { class Mpc; }

namespace vmpc_juce::gui::bitmap {

struct Background : public juce::Component {

public:
  Background(mpc::Mpc&);
  
  void paint(juce::Graphics& g) override;
  
  void mouseDown(const juce::MouseEvent& e) override {
    getParentComponent()->mouseDown(e);
  }
  
  void mouseUp(const juce::MouseEvent& e) override {
    getParentComponent()->mouseUp(e);
  }
  
  void mouseDrag(const juce::MouseEvent& e) override {
    getParentComponent()->mouseDrag(e);
  }
  
private:
  juce::Image img;

};

} // namespace vmpc_juce::gui::bitmap
