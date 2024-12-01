#pragma once

#include <cmath>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

#include "SvgComponent.hpp"
#include "DataWheelLines.hpp"

namespace vmpc_juce::gui::vector {

    class DataWheel : public juce::Component {
        public:
            DataWheel(
                    juce::Component *commonParentWithShadowToUse,
                    const float shadowSizeToUse,
                    const std::function<float()> &getScaleToUse)
                : commonParentWithShadow(commonParentWithShadowToUse), shadowSize(shadowSizeToUse), getScale(getScaleToUse)
            {
                backgroundSvg = new SvgComponent("data_wheel_without_dimple_and_lines.svg", commonParentWithShadow, shadowSize, getScale);
                addAndMakeVisible(backgroundSvg);

                lines = new DataWheelLines(getScale);
                addAndMakeVisible(lines);

                dimpleSvg = new SvgComponent("data_wheel_dimple.svg", commonParentWithShadow, 0.f, getScale);
                addAndMakeVisible(dimpleSvg);

                backgroundSvg->setInterceptsMouseClicks(false, false);
                lines->setInterceptsMouseClicks(false, false);
                dimpleSvg->setInterceptsMouseClicks(false, false);
            }

            ~DataWheel() override
            {
                delete backgroundSvg;
                delete lines;
                delete dimpleSvg;
            }

            juce::Rectangle<float> getDrawableBounds()
            {
                return backgroundSvg->getDrawableBounds();
            }

            void handleAngleChanged()
            {
                const auto drawableBounds = dimpleSvg->getDrawableBounds();
                const auto scale = getScale();
                const auto width = drawableBounds.getWidth() * scale;
                const auto height = drawableBounds.getHeight() * scale;

                const auto centerX = getWidth() / 2.0f;
                const auto centerY = getHeight() / 2.0f;

                const auto radius = std::min(getWidth(), getHeight()) * 0.27f;
                const auto xPos = centerX + std::cos(angle + juce::MathConstants<float>::pi) * radius - width / 2.0f;
                const auto yPos = centerY + std::sin(angle + juce::MathConstants<float>::pi) * radius - height / 2.0f;

                dimpleSvg->setBounds(std::round<float>(xPos), std::round<float>(yPos), width, height);

                lines->setAngle(angle);

                repaint();
            }

            void mouseUp(const juce::MouseEvent &) override
            {
                previousDragDistanceY = std::numeric_limits<int32_t>::max();
            }

            void mouseDrag(const juce::MouseEvent &e) override
            {
                if (previousDragDistanceY == std::numeric_limits<int32_t>::max())
                {
                    previousDragDistanceY = 0;
                }

                const auto distanceToProcessInPixels = e.getDistanceFromDragStartY() - previousDragDistanceY;
                previousDragDistanceY = e.getDistanceFromDragStartY();

                const auto fractionToAdd = -((float) distanceToProcessInPixels / getWidth());

                angle = fmod(angle + (float) fractionToAdd, juce::MathConstants<float>::twoPi);

                handleAngleChanged();
            }

            void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override
            {
                const auto increment = -wheel.deltaY;

                angle = fmod(angle + increment, juce::MathConstants<float>::twoPi);
                handleAngleChanged();
            }

            void resized() override
            {
                backgroundSvg->setSize(getWidth(), getHeight());
                lines->setSize(getWidth(), getHeight());
                handleAngleChanged();
            }

            SvgComponent *backgroundSvg = nullptr;

        private:
            SvgComponent *dimpleSvg = nullptr;
            DataWheelLines *lines = nullptr;
            juce::Component *commonParentWithShadow;
            const float shadowSize;
            const std::function<float()> &getScale;
            float angle = 0.f;
            int32_t previousDragDistanceY = std::numeric_limits<int32_t>::max();
    };

} // namespace vmpc_juce::gui::vector
