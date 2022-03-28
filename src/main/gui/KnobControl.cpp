#include "KnobControl.hpp"
#include <hardware/Pot.hpp>

static inline void clampIndex(int& knobIndex) {
	if (knobIndex < 0) {
		knobIndex = 0;
	}
	else if (knobIndex > 99) {
		knobIndex = 99;
	}
}

KnobControl::KnobControl(int type, std::weak_ptr<mpc::hardware::Pot> _pot)
: pot (_pot),
knobType (type),
knobIndex(_pot.lock()->getValue())
{
	clampIndex(knobIndex);
	repaint();
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
	knobIndex = pot.lock()->getValue();
	clampIndex(knobIndex);
	repaint();
}

void KnobControl::paint(juce::Graphics& g)
{
	if (knobs.isValid())
	{
		int imageWidth = getWidth();
		int imageHeight = getHeight();

		g.drawImage(knobs, 0, 0, imageWidth, imageHeight, 0, knobIndex * frameHeight, frameWidth, frameHeight);
	}
}

void KnobControl::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    auto p = pot.lock();
    mouseWheelControllable.processWheelEvent(wheel, [&](int increment) {     p->setValue(p->getValue() + increment);
        knobIndex = p->getValue();
        clampIndex(knobIndex);
        repaint();
    });
}
