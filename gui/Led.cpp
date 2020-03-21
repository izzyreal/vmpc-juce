#include "Led.hpp"

#include <string>

using namespace std;

Led::Led(Image led, Rectangle<float> rect)
	: VmpcComponent("led")
{
	this->led = led;
	this->rect = rect;
}

void Led::setBounds() {
	Component::setBounds(rect.getX(), rect.getY(), led.getWidth(), led.getHeight());
}

void Led::paint(Graphics& g) {
	if (on) {
		g.drawImage(led, 0, 0, led.getWidth(), led.getHeight(), 0, 0, led.getWidth(), led.getHeight());
	}
}

void Led::setOn(bool b)
{
	if (on == b) return;
	const MessageManagerLock mml(Thread::getCurrentThread());
	if (!mml.lockWasGained())
		return;
	on = b;
	repaint();
}

Led::~Led() {
}
