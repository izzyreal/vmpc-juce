#pragma once

#include "SvgComponent.hpp"
#include "hardware2/HardwareComponent.h"
#include <limits>

namespace vmpc_juce::gui::vector {
    class Pot : public SvgComponent {
        public:
            enum PotType { REC_GAIN, MAIN_VOLUME };

            Pot(std::shared_ptr<mpc::hardware2::Pot> modelToUse,
                    const PotType potTypeToUse,
                    juce::Component *commonParentWithShadowToUse,
                    const std::function<float()> &getScaleToUse)
                : SvgComponent({ potTypeToUse == REC_GAIN ? "rec_gain.svg" : "main_volume.svg" },
                        commonParentWithShadowToUse, 5.f,
                        getScaleToUse),
                potType(potTypeToUse),
                model(modelToUse)
                {
                }

            void sharedTimerCallback()
            {
                const float currentValue = model->getValue();

                if (lastValue != currentValue)
                {
                    handleAngleChanged(currentValue);
                    lastValue = currentValue;
                }
            }

            PotType potType;

        private:
            std::shared_ptr<mpc::hardware2::Pot> model;
            float lastValue = std::numeric_limits<float>::max();

            void handleAngleChanged(const float newAngle)
            {
                float startAngle = juce::MathConstants<float>::pi * 2.6f / 4.0f + juce::MathConstants<float>::halfPi;
                float endAngle = juce::MathConstants<float>::pi * 9.4f / 4.0f + juce::MathConstants<float>::halfPi;
                float angle = startAngle + newAngle * (endAngle - startAngle);

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
