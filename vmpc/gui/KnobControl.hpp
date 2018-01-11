#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "../gui/VmpcComponent.h"

#include <memory>

//#include <observer/Observer.hpp>

namespace mpc {
	namespace hardware {
		class Pot;
	}
}

class KnobControl
	: public VmpcComponent	
	{

private:
	std::weak_ptr<mpc::hardware::Pot> pot;
	int knobIndex{ 0 };
	int knobType{ 0 }; // 0 = rec, 1 = vol

public:
	void paint(Graphics& g) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;

	public:
	void setImage(Image image);

private:
	Image knobs;
	int frameWidth, frameHeight, lastDy = 0;

public:
	KnobControl(int type, std::weak_ptr<mpc::hardware::Pot> pot, int startIndex, const String& componentName = String::empty);
	~KnobControl();

};
