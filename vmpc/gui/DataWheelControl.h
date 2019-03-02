#include "../JuceLibraryCode/JuceHeader.h"
#include "../gui/VmpcComponent.h"

#include <observer/Observer.hpp>
#include <hardware/DataWheel.hpp>

class DataWheelControl 
	: public VmpcComponent
	, public moduru::observer::Observer
{
public:
	DataWheelControl(std::weak_ptr<mpc::hardware::DataWheel> dataWheel, const String& componentName = "");

	~DataWheelControl();
	void setImage(Image image, int numFrames);
	int getFrameWidth() const { return frameWidth; }
	int getFrameHeight() const { return frameHeight; }
	void paint(Graphics& g) override;

	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;

public:
	void update(moduru::observer::Observable* o, std::any arg) override;

private:
	Image filmStripImage;
	int numFrames;
	int frameWidth, frameHeight;

	int dataWheelIndex = 0;
	int lastDy = 0;
	std::weak_ptr<mpc::hardware::DataWheel> dataWheel;
};
