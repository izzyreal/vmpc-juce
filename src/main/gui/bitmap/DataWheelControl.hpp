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
#include "VmpcTooltipComponent.hpp"

#include <hardware/DataWheel.hpp>

#include "gui/MouseWheelControllable.hpp"

#include <set>

namespace vmpc_juce::gui::bitmap {
    class DataWheelControl 
        : public VmpcTooltipComponent
    {
        public:
            DataWheelControl(mpc::Mpc& mpc, std::weak_ptr<mpc::hardware::DataWheel> dataWheel);

            ~DataWheelControl() override;
            void setImage(juce::Image image, int numFrames);
            int getFrameWidth() const { return frameWidth; }
            int getFrameHeight() const { return frameHeight; }
            void paint(juce::Graphics& g) override;

            void mouseDrag(const juce::MouseEvent&) override;
            void mouseDown(const juce::MouseEvent&) override;
            void mouseUp(const juce::MouseEvent&) override;
            void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

            void updateUI(int increment);

        private:
            vmpc_juce::gui::MouseWheelControllable mouseWheelControllable;
            juce::Image filmStripImage;
            int numFrames;
            int frameWidth, frameHeight;

            std::set<int> mouseDownEventSources;
            juce::Time latestMouseDownTime = juce::Time(0);
            int dataWheelIndex = 0;
            float lastDy = 0;
            double pixelCounter = 0;
            double fineSensitivity = 0.06;
            std::weak_ptr<mpc::hardware::DataWheel> dataWheel;
    };
} // vmpc_juce::gui::bitmap
