#include "VmpcComponent.h"

#include <observer/Observer.hpp>
#include <hardware/DataWheel.hpp>

class DataWheelControl 
	: public VmpcComponent
	, public moduru::observer::Observer
{
public:
	DataWheelControl(std::weak_ptr<mpc::hardware::DataWheel> dataWheel);

	~DataWheelControl() override;
	void setImage(juce::Image image, int numFrames);
	int getFrameWidth() const { return frameWidth; }
	int getFrameHeight() const { return frameHeight; }
	void paint(juce::Graphics& g) override;

	void mouseDrag(const juce::MouseEvent&) override;
	void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

	void update(moduru::observer::Observable* o, nonstd::any arg) override;

private:
    juce::Image filmStripImage;
	int numFrames;
	int frameWidth, frameHeight;

	int dataWheelIndex = 0;
	int lastDy = 0;
	double pixelCounter = 0;
	double fineSensitivity = 0.06;
	std::weak_ptr<mpc::hardware::DataWheel> dataWheel;
};
