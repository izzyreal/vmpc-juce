#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "../gui/VmpcComponent.h"

#include <memory>

namespace mpc {
	namespace hardware {
		class Slider;
	}
}

class SliderControl
	: public VmpcComponent
	{

private:
	std::weak_ptr<mpc::hardware::Slider> slider;
	int sliderIndex{ 0 };

private:
	Image filmStripImage;
	int frameWidth, frameHeight;
	int lastDy = 0;

public:
	void setImage(Image image);

public:
	void paint(Graphics& g) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;

public:
	SliderControl(std::weak_ptr<mpc::hardware::Slider> slider, int startIndex, const String& componentName);
	~SliderControl();

};
