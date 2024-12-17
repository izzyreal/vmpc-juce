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

#include <juce_gui_basics/juce_gui_basics.h>
#include "juce_audio_processors/juce_audio_processors.h"

#include "TooltipOverlay.hpp"
#include "Knob.hpp"
#include "Slider.hpp"

#include "Mpc.hpp"
#include "hardware/Hardware.hpp"
#include "hardware/Button.hpp"
#include "hardware/HwPad.hpp"
#include "hardware/DataWheel.hpp"
#include "hardware/Pot.hpp"
#include "hardware/HwSlider.hpp"
#include "controls/KbMapping.hpp"

namespace vmpc_juce::gui::vector {

    template<class ComponentClass>
        static ComponentClass* getChildComponentOfClass(juce::Component *parent)
        {
            for (int i = 0; i < parent->getNumChildComponents(); ++i)
            {
                auto* childComp = parent->getChildComponent(i);

                if (auto c = dynamic_cast<ComponentClass*> (childComp))
                    return c;

                if (auto c = getChildComponentOfClass<ComponentClass> (childComp))
                    return c;
            }

            return nullptr;
        }

    class MpcHardwareMouseListener : public juce::MouseListener {
        public:
            MpcHardwareMouseListener(mpc::Mpc &mpcToUse, const std::string labelToUse) : label(labelToUse), mpc(mpcToUse) {}

            void mouseMove(const juce::MouseEvent &e) override
            {
                const auto currentModifiers = juce::ModifierKeys::getCurrentModifiers();

                if (!currentModifiers.isAnyModifierKeyDown())
                {
                    return;
                }

                setKeyTooltipVisibility(e.eventComponent, true);
            }

            void mouseExit(const juce::MouseEvent &e) override
            {
                setKeyTooltipVisibility(e.eventComponent, false);
            }

            void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
            {
                setKeyTooltipVisibility(event.eventComponent, false);

                if (label == "rec_gain" || label == "main_volume")
                {
                    auto pot = label == "rec_gain" ? mpc.getHardware()->getRecPot() : mpc.getHardware()->getVolPot();
                    auto knob = dynamic_cast<Knob*>(event.eventComponent);
                    pot->setValue(knob->getAngleFactor() * 100);
                }
                else if (label == "slider")
                {
                    syncMpcSliderModelWithUi(event.eventComponent);
                }
            }

            void mouseDown(const juce::MouseEvent &e) override
            {
                setKeyTooltipVisibility(e.eventComponent, false);

                if (label.length() >= 4 && label.substr(0, 4) == "pad-")
                {
                    const auto digitsString = label.substr(4);
                    const auto padNumber = std::stoi(digitsString);
                    auto pad = mpc.getHardware()->getPad(padNumber - 1);
                    const auto velocity = 127 - ((e.position.getY() / e.eventComponent->getBounds().getHeight()) * 127.f);
                    pad->push(velocity);
                    return;
                }
                else if (label == "cursor")
                {
                    juce::Path left, top, bottom, right;

                    left.startNewSubPath(0.f, 0.f);
                    left.lineTo(0.2f, 0.f);
                    left.lineTo(0.25f, 0.5f);
                    left.lineTo(0.2f, 1.f);
                    left.lineTo(0.f, 1.f);
                    left.closeSubPath();

                    top.startNewSubPath(0.2f, 0.f);
                    top.lineTo(0.8f, 0.f);
                    top.lineTo(0.75f, 0.5f);
                    top.lineTo(0.25f, 0.5f);
                    top.lineTo(0.2f, 0.f);
                    top.closeSubPath();

                    right = left;
                    right.applyTransform(juce::AffineTransform(-1.0f, 0.0f, 1.f, 0.0f, 1.0f, 0.0f));

                    bottom = top;
                    bottom.applyTransform(juce::AffineTransform().verticalFlip(1.f));

                    const auto compWidth = e.eventComponent->getWidth();
                    const auto compHeight = e.eventComponent->getHeight();
                    juce::AffineTransform scaleTransform = juce::AffineTransform().scaled(compWidth, compHeight);

                    left.applyTransform(scaleTransform);
                    top.applyTransform(scaleTransform);
                    right.applyTransform(scaleTransform);
                    bottom.applyTransform(scaleTransform);

                    if (left.contains(e.position))
                    {
                        mpc.getHardware()->getButton("left")->push();
                    }
                    else if (top.contains(e.position))
                    {
                        mpc.getHardware()->getButton("up")->push();
                    }
                    else if (right.contains(e.position))
                    {
                        mpc.getHardware()->getButton("right")->push();
                    }
                    else if (bottom.contains(e.position))
                    {
                        mpc.getHardware()->getButton("down")->push();
                    }

                    return;
                }
                else if (label == "data-wheel" || label == "rec_gain" || label == "main_volume" || label == "slider")
                {
                    return;
                }

                mpc.getHardware()->getButton(label)->push();
            }

            void mouseUp(const juce::MouseEvent &e) override
            {
                if (label.length() >= 4 && label.substr(0, 4) == "pad-")
                {
                    const auto digitsString = label.substr(4);
                    const auto padNumber = std::stoi(digitsString);
                    auto pad = mpc.getHardware()->getPad(padNumber - 1);
                    pad->release();
                }
                else if (label == "rec_gain" || label == "main_volume")
                {
                }
                else if (label == "cursor" || label == "slider")
                {
                }
                else if (label == "data-wheel")
                {
                }
                else
                {
                    mpc.getHardware()->getButton(label)->release();
                }
            }

            void mouseDrag(const juce::MouseEvent &e) override
            {
                if (label == "rec_gain" || label == "main_volume")
                {
                    auto pot = label == "rec_gain" ? mpc.getHardware()->getRecPot() : mpc.getHardware()->getVolPot();
                    auto knob = dynamic_cast<Knob*>(e.eventComponent);
                    pot->setValue(knob->getAngleFactor() * 100.f);
                }
                else if (label == "slider")
                {
                    syncMpcSliderModelWithUi(e.eventComponent);
                }
            }

        private:
            mpc::Mpc &mpc;
            const std::string label;

            void setKeyTooltipVisibility(juce::Component *c, const bool visibleEnabled)
            {
                const auto editor = c->findParentComponentOfClass<juce::AudioProcessorEditor>();
                auto tooltipOverlay = getChildComponentOfClass<TooltipOverlay>(editor);
                
                if (tooltipOverlay == nullptr)
                {
                    return;
                }
                
                tooltipOverlay->setKeyTooltipVisibility(label, visibleEnabled);
            }

            void syncMpcSliderModelWithUi(juce::Component *eventComponent)
            {
                const auto sliderComponent = dynamic_cast<Slider*>(eventComponent);

                if (sliderComponent == nullptr)
                {
                    return;
                }

                auto hwSlider = mpc.getHardware()->getSlider();
                const auto yPosFraction = sliderComponent->getSliderYPosFraction();
                hwSlider->setValue(yPosFraction * 127.f);
            }
    };

} // namespace vmpc_juce::gui::vector
