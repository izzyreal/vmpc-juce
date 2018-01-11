#include "KnobControl.hpp"
#include <Logger.hpp>
#include <hardware/Pot.hpp>


static inline void clampIndex(int& knobIndex) {
	if (knobIndex < 0) {
		knobIndex = 0;
	}
	else if (knobIndex > 99) {
		knobIndex = 99;
	}
}

KnobControl::KnobControl(int type, std::weak_ptr<mpc::hardware::Pot> pot, int startIndex, const String &componentName)
	: VmpcComponent(componentName)
{
	this->knobs = knobs;
	knobType = type;
	knobIndex = startIndex;
	this->pot = pot;
	knobIndex = pot.lock()->getValue();
	clampIndex(knobIndex);
	repaint();
}

void KnobControl::setImage(Image image)
{
	knobs = image;
	frameHeight = knobs.getHeight() / 100;
	frameWidth = knobs.getWidth();
	repaint();
}

void KnobControl::mouseUp(const MouseEvent& event) {
	lastDy = 0;
	Component::mouseUp(event);
}

void KnobControl::mouseDrag(const MouseEvent& event) {
	auto dY = event.getDistanceFromDragStartY() - lastDy;
	lastDy = event.getDistanceFromDragStartY();
	pot.lock()->setValue(pot.lock()->getValue() + dY);
	knobIndex = pot.lock()->getValue();
	clampIndex(knobIndex);
	repaint();
}

void KnobControl::paint(Graphics& g)
{
	if (knobs.isValid())
	{
		int imageWidth = getWidth();
		int imageHeight = getHeight();

		g.drawImage(knobs, 0, 0, imageWidth, imageHeight, 0, knobIndex * frameHeight, frameWidth, frameHeight);
	}
}

KnobControl::~KnobControl() {
}
