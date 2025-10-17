#pragma once

#include <cmath>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <limits>

#include "SvgComponent.hpp"
#include "DataWheelLines.hpp"

#include "hardware/HardwareComponent.h"

namespace vmpc_juce::gui::vector {

    class DataWheel : public juce::Component {
        public:
            DataWheel(std::shared_ptr<mpc::hardware::DataWheel> dataWheelModelToUse, juce::Component *commonParentWithShadowToUse,
                      const float shadowSizeToUse,
                      const std::function<float()> &getScaleToUse)
                : commonParentWithShadow(commonParentWithShadowToUse),
                shadowSize(shadowSizeToUse),
                getScale(getScaleToUse),
                dataWheelModel(dataWheelModelToUse)
            {
                backgroundSvg = new SvgComponent({"data_wheel_without_dimple_and_lines.svg"}, commonParentWithShadow, shadowSize, getScale);
                addAndMakeVisible(backgroundSvg);

                lines = new DataWheelLines(getScale);
                addAndMakeVisible(lines);

                dimpleSvg = new SvgComponent({"data_wheel_dimple.svg"}, commonParentWithShadow, 0.f, getScale);
                addAndMakeVisible(dimpleSvg);

                backgroundSvg->setInterceptsMouseClicks(false, false);
                lines->setInterceptsMouseClicks(false, false);
                dimpleSvg->setInterceptsMouseClicks(false, false);
            }

            void sharedTimerCallback()
            {
                const float targetAngle = dataWheelModel->getAngle(); // unwrapped, may be >> 1 or << -1
                const float diff = targetAngle - displayAngle;

                const float animationSpeedToUse = std::abs(diff) > 0.02f ? animationSpeed : 100.f;
                // Smoothly move displayAngle toward targetAngle
                const float step = diff * juce::jmin(animationSpeedToUse * 0.016f, 1.0f);
                displayAngle += step;

                // Convert to [0,1) only when drawing
                handleAngleChanged(std::fmod(displayAngle, 1.f));
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

            void resized() override
            {
                backgroundSvg->setSize(getWidth(), getHeight());
                lines->setSize(getWidth(), getHeight());
                handleAngleChanged(dataWheelModel->getAngle());
            }

            SvgComponent *backgroundSvg = nullptr;

        private:
            std::shared_ptr<mpc::hardware::DataWheel> dataWheelModel;
            float lastAngle = std::numeric_limits<float>::max();
            SvgComponent *dimpleSvg = nullptr;
            DataWheelLines *lines = nullptr;
            juce::Component *commonParentWithShadow;
            const float shadowSize;
            const std::function<float()> &getScale;

            float displayAngle = 0.0f;   // the angle currently being *drawn*
            float animationSpeed = 20.0f; // larger = faster interpolation

            void handleAngleChanged(const float newAngle)
            {
                const auto drawableBounds = dimpleSvg->getDrawableBounds();
                const auto scale = getScale();
                const auto width = drawableBounds.getWidth() * scale;
                const auto height = drawableBounds.getHeight() * scale;

                const auto centerX = getWidth() / 2.0f;
                const auto centerY = getHeight() / 2.0f;

                const auto radius = std::min(getWidth(), getHeight()) * 0.27f;
                
                const auto theta = newAngle * juce::MathConstants<float>::twoPi;
                const auto xPos = centerX + std::sin(theta) * radius - width / 2.0f;
                const auto yPos = centerY - std::cos(theta) * radius - height / 2.0f;

                dimpleSvg->setBounds(static_cast<int>(xPos), static_cast<int>(yPos), static_cast<int>(width), static_cast<int>(height));

                lines->setAngle(newAngle);
                repaint();
            }


    };

} // namespace vmpc_juce::gui::vector
