#pragma once

#include <cmath>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

#include "SvgComponent.hpp"
#include "DataWheelLines.hpp"

#include "gui/MouseWheelControllable.hpp"

#include "Mpc.hpp"
#include "hardware2/Hardware2.h"
#include "hardware2/HardwareComponent.h"
#include "inputlogic/HardwareTranslator.h"
#include "inputlogic/InputMapper.h"
#include "inputlogic/InputAction.h"

namespace vmpc_juce::gui::vector {

    class DataWheel : public juce::Component {
        public:
            DataWheel(mpc::Mpc &mpcToUse,
                    juce::Component *commonParentWithShadowToUse,
                    const float shadowSizeToUse,
                    const std::function<float()> &getScaleToUse)
                : mpc(mpcToUse), commonParentWithShadow(commonParentWithShadowToUse), shadowSize(shadowSizeToUse), getScale(getScaleToUse)
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
                /*
                mpc.getHardware()->getDataWheel()->updateUi = [this](int increment) {
                    juce::MessageManager::callAsync([this, increment] {
                            setAngle(getAngle() + (increment * 0.02f));
                            });
                };
                */
            }

            ~DataWheel() override
            {
                //mpc.getHardware2()->getDataWheel()->updateUi = {};
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

                dimpleSvg->setBounds(std::round(xPos), std::round(yPos), width, height);

                lines->setAngle(angle);

                repaint();
            }

            void mouseDown(const juce::MouseEvent &event) override
            {
                mouseDownEventSources.emplace(event.source.getIndex());

                if (latestMouseDownTime == juce::Time(0))
                {
                    latestMouseDownTime = event.mouseDownTime;
                }
            }

            void mouseUp(const juce::MouseEvent &event) override
            {
                mouseDownEventSources.erase(event.source.getIndex());

                if (mouseDownEventSources.empty())
                {
                    latestMouseDownTime = juce::Time(0);
                }

                lastDy = 0;
            }

            void mouseDrag(const juce::MouseEvent &event) override
            {
                if (mouseDownEventSources.size() > 1 && event.source.getLastMouseDownTime() != latestMouseDownTime)
                {
                    return;
                }

                auto dY = -(event.getDistanceFromDragStartY() - lastDy);

                if (dY == 0)
                {
                    return;
                }

                const bool iOS = juce::SystemStats::getOperatingSystemType() == juce::SystemStats::OperatingSystemType::iOS;

                auto dataWheel = mpc.getHardware2()->getDataWheel();

                if (event.mods.isAnyModifierKeyDown() || iOS)
                {
                    float iOSMultiplier = 1.0;

                    for (int i = 1; i < mouseDownEventSources.size(); i++)
                    {
                        iOSMultiplier *= 10.f;
                    }

                    pixelCounter += (dY * fineSensitivity * (iOS ? iOSMultiplier : 1.0));
                    auto candidate = static_cast<int>(pixelCounter);
                    if (candidate >= 1 || candidate <= -1)
                    {
                        pixelCounter -= candidate;
                        dataWheel->turn(candidate);
                        mpc.inputMapper.trigger(mpc::inputlogic::HardwareTranslator::fromDataWheelTurn(candidate));
                    }

                }
                else
                {
                    dataWheel->turn(dY);
                    //mpc.inputMapper.trigger(mpc::inputlogic::HardwareTranslator::fromDataWheelTurn(dY));
                }

                lastDy = event.getDistanceFromDragStartY();
            }

            void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override
            {
                auto dw = mpc.getHardware2()->getDataWheel();
                auto &inputMapper = mpc.inputMapper;
                mouseWheelControllable.processWheelEvent(wheel, [&dw, &inputMapper](int increment) {
                        inputMapper.trigger(mpc::inputlogic::HardwareTranslator::fromDataWheelTurn(increment));
                        dw->turn(increment);
                        });
            }

            void resized() override
            {
                backgroundSvg->setSize(getWidth(), getHeight());
                lines->setSize(getWidth(), getHeight());
                handleAngleChanged();
            }

            SvgComponent *backgroundSvg = nullptr;

            float getAngle() { return angle; }
            void setAngle(float newAngle) { angle = newAngle; handleAngleChanged(); }

        private:
            vmpc_juce::gui::MouseWheelControllable mouseWheelControllable;
            mpc::Mpc &mpc;
            std::set<int> mouseDownEventSources;
            SvgComponent *dimpleSvg = nullptr;
            DataWheelLines *lines = nullptr;
            juce::Component *commonParentWithShadow;
            const float shadowSize;
            const std::function<float()> &getScale;
            float angle = 0.f;
            float lastDy = 0.f;
            double pixelCounter = 0;
            double fineSensitivity = 0.06;
            juce::Time latestMouseDownTime = juce::Time(0);
    };

} // namespace vmpc_juce::gui::vector
