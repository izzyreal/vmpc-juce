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
#include "KnobControl.hpp"
#include <hardware/Pot.hpp>

using namespace vmpc_juce::gui::bitmap;

static inline int clampIndex(int knobIndex) {
	if (knobIndex < 0) {
		knobIndex = 0;
	}
	else if (knobIndex > 99) {
		knobIndex = 99;
	}
    return knobIndex;
}

KnobControl::KnobControl(std::weak_ptr<mpc::hardware::Pot> _pot)
: pot (_pot)
{
    pot.lock()->updateUi = [this]() {
        juce::MessageManager::callAsync ([this] {
			auto knobControl = dynamic_cast<KnobControl*>(this);
            if (knobControl != nullptr) knobControl->repaint();
        });
    };
}

KnobControl::~KnobControl()
{
    pot.lock()->updateUi = [](){};
}


void KnobControl::setImage(juce::Image image)
{
	knobs = image;
	frameHeight = knobs.getHeight() / 100;
	frameWidth = knobs.getWidth();
	repaint();
}

void KnobControl::mouseUp(const juce::MouseEvent& event) {
	lastDy = 0;
	Component::mouseUp(event);
}

void KnobControl::mouseDrag(const juce::MouseEvent& event) {
	auto dY = -(event.getDistanceFromDragStartY() - lastDy);
	lastDy = event.getDistanceFromDragStartY();
	pot.lock()->setValue(pot.lock()->getValue() + dY);
}

void KnobControl::paint(juce::Graphics& g)
{
	if (knobs.isValid())
	{
		int imageWidth = getWidth();
		int imageHeight = getHeight();

        auto knobIndex = clampIndex(pot.lock()->getValue());
        g.drawImage(knobs, 0, 0, imageWidth, imageHeight, 0, knobIndex * frameHeight, frameWidth, frameHeight);
	}
}

void KnobControl::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    auto p = pot.lock();
    mouseWheelControllable.processWheelEvent(wheel, [&](int increment) {
        p->setValue(p->getValue() + increment);
    });
}
