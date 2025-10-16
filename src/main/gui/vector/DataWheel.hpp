#pragma once

#include <cmath>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <limits>

#include "SvgComponent.hpp"
#include "DataWheelLines.hpp"

#include "hardware2/HardwareComponent.h"

namespace vmpc_juce::gui::vector {

    class DataWheel : public juce::Component {
        public:
            DataWheel(std::shared_ptr<mpc::hardware2::DataWheel> dataWheelModelToUse, juce::Component *commonParentWithShadowToUse,
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
                const float currentAngle = dataWheelModel->getAngle();

                if (lastAngle != currentAngle)
                {
                    handleAngleChanged(currentAngle);
                    lastAngle = currentAngle;
                }
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
            std::shared_ptr<mpc::hardware2::DataWheel> dataWheelModel;
            float lastAngle = std::numeric_limits<float>::max();
            SvgComponent *dimpleSvg = nullptr;
            DataWheelLines *lines = nullptr;
            juce::Component *commonParentWithShadow;
            const float shadowSize;
            const std::function<float()> &getScale;

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

                dimpleSvg->setBounds(std::round(xPos), std::round(yPos), width, height);

                lines->setAngle(newAngle);
                repaint();
            }


    };

} // namespace vmpc_juce::gui::vector
