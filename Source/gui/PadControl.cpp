#include "PadControl.hpp"
#include <hardware/HwPad.hpp>

#include <math.h>

using namespace std;

PadControl::PadControl(Rectangle <float> rect, std::weak_ptr<mpc::hardware::HwPad> pad, Image padhit, const String &componentName)
	: VmpcComponent(componentName)
{
	this->pad = pad;
	this->padhitImg = padhit;
	this->rect = rect;
	pad.lock()->addObserver(this);
}

void PadControl::timerCallback() {
	if (fading) padhitBrightness -= 4;
	if (padhitBrightness < 0) {
		padhitBrightness = 0;
		repaint();
		fading = false;
		stopTimer();
	}
	else {
		repaint();
	}
}

void PadControl::update(moduru::observer::Observable* o, nonstd::any arg) {
	int velocity = nonstd::any_cast<int>(arg);
	if (velocity == 255) {
		fading = true;
		pressed = false;
	}
	else {
		padhitBrightness = velocity + 25;
		pressed = true;
		fading = false;
		startTimer(20);
	}
}

int PadControl::getVelo(int x, int y) {
	float centX = rect.getCentreX() - rect.getX();
	float centY = rect.getCentreY() - rect.getY();
	float distX = x - centX;
	float distY = y - centY;
	float powX = pow(distX, 2);
	float powY = pow(distY, 2);
	float dist = sqrt(powX + powY);
	if (dist > 46) dist = 46;
	int velo = 127 - (dist * (127.0 / 48.0));
	return velo;
}

void PadControl::mouseDown(const MouseEvent& event) {
	pad.lock()->push(getVelo(event.x, event.y));
}

void PadControl::mouseDoubleClick(const MouseEvent& event) {
}

void PadControl::mouseUp(const MouseEvent& event) {
	pad.lock()->release();
}

void PadControl::setBounds() {
	setSize(rect.getWidth(), rect.getHeight());
	Component::setBounds(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void PadControl::paint(Graphics& g)
{
	auto img = padhitImg.createCopy();
	float mult = (float)(padhitBrightness) / 150.0;
	img.multiplyAllAlphas(mult);
	g.drawImageAt(img, 0, 0);
}

PadControl::~PadControl() {
	pad.lock()->deleteObserver(this);
}
