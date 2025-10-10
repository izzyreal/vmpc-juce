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

        private:
            float angleFactor = 0.f;
            
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
