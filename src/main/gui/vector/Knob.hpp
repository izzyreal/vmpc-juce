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

namespace vmpc_juce::gui::vector {
    class Knob : public SvgComponent {
        public:
            enum KnobType { REC_GAIN, MAIN_VOLUME };

            Knob(const KnobType knobTypeToUse, juce::Component *commonParentWithShadowToUse, const std::function<float()> &getScaleToUse)
                : SvgComponent({ knobTypeToUse == REC_GAIN ? "rec_gain.svg" : "main_volume.svg" }, commonParentWithShadowToUse, 5.f, getScaleToUse)
                {
                    handleAngleChanged();
                }

            void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel) override
            {
                const auto increment = -wheel.deltaY;
                angleFactor = std::clamp<float>(angleFactor + increment, 0, 1);
                handleAngleChanged();
            }

            void mouseUp(const juce::MouseEvent &) override
            {
                previousDragDistanceY = previousDragDistanceY = std::numeric_limits<int32_t>::max();
            }

            void mouseDrag(const juce::MouseEvent &e) override
            {
                if (previousDragDistanceY == std::numeric_limits<int32_t>::max())
                {
                    previousDragDistanceY = 0;
                }

                const auto increment = float(e.getDistanceFromDragStartY() - previousDragDistanceY) * 0.005f * -1.f;

                previousDragDistanceY = e.getDistanceFromDragStartY();

                angleFactor = std::clamp<float>(angleFactor + (float) increment, 0, 1);
                handleAngleChanged();
            }

            void setAngleFactor(float newAngleFactor)
            {
                angleFactor = newAngleFactor;
                handleAngleChanged();
            }

            float getAngleFactor() { return angleFactor; }

        private:
            float angleFactor = 0.f;
            int32_t previousDragDistanceY = std::numeric_limits<int32_t>::max();

            void handleAngleChanged()
            {
                float startAngle = juce::MathConstants<float>::pi * 2.6f / 4.0f + juce::MathConstants<float>::halfPi;
                float endAngle = juce::MathConstants<float>::pi * 9.4f / 4.0f + juce::MathConstants<float>::halfPi;
                float angle = startAngle + angleFactor * (endAngle - startAngle);

                auto bounds = getDrawableBounds();
                auto centerX = bounds.getCentreX();
                auto centerY = bounds.getCentreY();

                getCurrentDrawable()->setTransform(juce::AffineTransform()
                        .translated(-centerX, -centerY)
                        .rotated(angle)
                        .translated(centerX, centerY));

                repaint();
            }
    };
} // namespace vmpc_juce::gui::vector
