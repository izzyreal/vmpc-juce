#pragma once

#include "SvgComponent.hpp"

class Knob : public SvgComponent {
    public:
        enum KnobType { REC_GAIN, MAIN_VOLUME };

        Knob(const KnobType knobTypeToUse, juce::Component *commonParentWithShadowToUse, const std::function<float()> &getScaleToUse)
            : SvgComponent(knobTypeToUse == REC_GAIN ? "rec_gain.svg" : "main_volume.svg", commonParentWithShadowToUse, 5.f, getScaleToUse)
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

    private:
        float angleFactor = 0.f;
        int32_t previousDragDistanceY = std::numeric_limits<int32_t>::max();

        void handleAngleChanged()
        {
            float startAngle = juce::MathConstants<float>::pi * 2.6f / 4.0f + juce::MathConstants<float>::halfPi;
            float endAngle = juce::MathConstants<float>::pi * 9.4f / 4.0f + juce::MathConstants<float>::halfPi;
            float angle = startAngle + angleFactor * (endAngle - startAngle);

            auto bounds = svgDrawable->getDrawableBounds();
            auto centerX = bounds.getCentreX();
            auto centerY = bounds.getCentreY();

            svgDrawable->setTransform(juce::AffineTransform()
                    .translated(-centerX, -centerY)
                    .rotated(angle)
                    .translated(centerX, centerY));

            repaint();
        }
};
