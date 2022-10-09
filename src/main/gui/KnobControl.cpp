#include "KnobControl.hpp"
#include <hardware/Pot.hpp>

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
        juce::MessageManager::callAsync ([target = juce::WeakReference<Component> { this }] {
            if (target != nullptr) target->repaint();
        });
    };
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
