#pragma once

#include <cmath>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

#include "SvgComponent.hpp"
#include "DataWheelLines.hpp"

#include "gui/MouseWheelControllable.hpp"

#include "Mpc.hpp"
#include "hardware2/Hardware2.h"
#include "inputlogic/HostInputEvent.h"

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
                return;

            auto dY = -(event.getDistanceFromDragStartY() - lastDy);
            if (dY == 0)
                return;

            const bool iOS = juce::SystemStats::getOperatingSystemType() == juce::SystemStats::OperatingSystemType::iOS;
            using namespace mpc::inputlogic;

            const float normX = event.position.getX() / static_cast<float>(event.eventComponent->getWidth());
            const float normY = event.position.getY() / static_cast<float>(event.eventComponent->getHeight());

            HostInputEvent hostEvent;
            hostEvent.source = HostInputEvent::MOUSE;
            hostEvent.payload = MouseEvent {
                MouseEvent::ButtonState{
                    event.mods.isLeftButtonDown(),
                    event.mods.isMiddleButtonDown(),
                    event.mods.isRightButtonDown()
                },
                MouseEvent::DATA_WHEEL,
                normX,
                normY,
                0.f,
                0.f,
                0.f,
                MouseEvent::DRAG
            };

            MouseEvent& mouseEvent = std::get<MouseEvent>(hostEvent.payload);

            if (event.mods.isAnyModifierKeyDown() || iOS)
            {
                float iOSMultiplier = 1.0f;
                for (int i = 1; i < mouseDownEventSources.size(); i++)
                    iOSMultiplier *= 10.f;

                pixelCounter += (dY * fineSensitivity * (iOS ? iOSMultiplier : 1.0));
                auto candidate = static_cast<int>(pixelCounter);
                if (candidate >= 1 || candidate <= -1)
                {
                    pixelCounter -= candidate;
                    mouseEvent.deltaY = static_cast<float>(candidate);
                    mpc.getHardware2()->dispatchHostInput(hostEvent);
                }
            }
            else
            {
                mouseEvent.deltaY = static_cast<float>(dY);
                mpc.getHardware2()->dispatchHostInput(hostEvent);
            }

            lastDy = event.getDistanceFromDragStartY();
        }

        void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override
        {
            using namespace mpc::inputlogic;

            mouseWheelControllable.processWheelEvent(wheel, [this](int increment) {
                HostInputEvent hostEvent;
                hostEvent.source = HostInputEvent::MOUSE;

                MouseEvent mouseEvent;
                mouseEvent.buttonState = { false, false, false };
                mouseEvent.guiElement = MouseEvent::DATA_WHEEL;
                mouseEvent.deltaX = 0.0f;
                mouseEvent.deltaY = 0.0f;
                mouseEvent.wheelDelta = static_cast<float>(increment);
                mouseEvent.type = MouseEvent::WHEEL;

                hostEvent.payload = mouseEvent;
                mpc.getHardware2()->dispatchHostInput(hostEvent);
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
