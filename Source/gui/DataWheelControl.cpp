#include "DataWheelControl.h"

#include <Logger.hpp>
using namespace std;

DataWheelControl::DataWheelControl(weak_ptr<mpc::hardware::DataWheel> dataWheel, const String &componentName)
	: VmpcComponent(componentName),
	numFrames(0),
	frameWidth(0),
	frameHeight(0)
{
	this->dataWheel = dataWheel;
}

void DataWheelControl::mouseUp(const MouseEvent& event) {
	lastDy = 0;
	Component::mouseUp(event);
}

void DataWheelControl::mouseDrag(const MouseEvent& event) {
	auto dY = event.getDistanceFromDragStartY() - lastDy;
	dataWheel.lock()->turn(dY);
	lastDy = event.getDistanceFromDragStartY();
}

static inline void clampIndex(int& dataWheelIndex) {
	if (dataWheelIndex < 0) {
		while (dataWheelIndex < 0)
			dataWheelIndex += 100;
	}
	else if (dataWheelIndex > 99) {
		while (dataWheelIndex > 99)
			dataWheelIndex -= 100;
	}
}

void DataWheelControl::update(moduru::observer::Observable* o, nonstd::any arg) {
	int increment = nonstd::any_cast<int>(arg);
	dataWheelIndex += increment;
	clampIndex(dataWheelIndex);
	repaint();
}

void DataWheelControl::setImage(Image image, int numFrames_)
{
	filmStripImage = image;
	numFrames = numFrames_;
	frameHeight = filmStripImage.getHeight() / numFrames;
	frameWidth = filmStripImage.getWidth();
	repaint();
}

DataWheelControl::~DataWheelControl()
{
	dataWheel.lock()->deleteObserver(this);
}

void DataWheelControl::paint(Graphics& g)
{
	if (filmStripImage.isValid())
	{
		int imageWidth = getWidth();
		int imageHeight = getHeight();

		g.drawImage(filmStripImage, 0, 0, imageWidth, imageHeight, 0, dataWheelIndex * frameHeight, frameWidth, frameHeight);
	}
}
