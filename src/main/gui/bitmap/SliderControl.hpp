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

#include "gui/MouseWheelControllable.hpp"

#include <memory>

namespace mpc::hardware {
    class Slider;
}

namespace vmpc_juce::gui::bitmap {
    class SliderControl
        : public VmpcComponent, public juce::Timer
    {

        private:
            MouseWheelControllable mouseWheelControllable;
            std::weak_ptr<mpc::hardware::Slider> slider;
            int sliderIndex{ 0 };

        private:
            juce::Image filmStripImage;
            int frameWidth, frameHeight;
            int lastDy = 0;

        public:
            void setImage(juce::Image image);

        public:
            void paint(juce::Graphics& g) override;
            void mouseDrag(const juce::MouseEvent& event) override;
            void mouseUp(const juce::MouseEvent& event) override;
            void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

            void timerCallback() override;

            SliderControl(std::weak_ptr<mpc::hardware::Slider> slider);
    };
} // namespace vmpc_juce::gui::bitmap
