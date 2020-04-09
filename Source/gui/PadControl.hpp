#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "VmpcComponent.h"

#include <observer/Observer.hpp>

#include <thread>
#include <memory>

namespace mpc {
	namespace hardware {
		class HwPad;
	}
}

class PadControl
	: public VmpcComponent
	, public Timer
	, public moduru::observer::Observer
	{

private:
	std::weak_ptr<mpc::hardware::HwPad> pad;
	int padhitBrightness = 0;

private:
	int getVelo(int x, int y);

	private:
		Image padhitImg;
		Rectangle <float> rect;

	public:
		void paint(Graphics& g) override;
		void mouseDown(const MouseEvent& event) override;
		void mouseUp(const MouseEvent& event) override;
		void mouseDoubleClick(const MouseEvent& event) override;
		void timerCallback() override;

	public:
		void update(moduru::observer::Observable* o, nonstd::any arg) override;

	public:
		void setBounds();

	private:
		bool fading = false;
		bool pressed = false;

public:
	PadControl(Rectangle<float> rect, std::weak_ptr<mpc::hardware::HwPad> pad, Image padhit, const String& componentName);
	~PadControl();

};
