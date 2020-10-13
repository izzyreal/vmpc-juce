#pragma once
#include "VmpcComponent.h"

#include <observer/Observer.hpp>

#include <thread>
#include <memory>

namespace mpc { class Mpc; }

namespace mpc::hardware
{
	class HwPad;
}

class PadControl
	: public VmpcComponent
	, public Timer
	, public FileDragAndDropTarget
	, public moduru::observer::Observer
{

private:
	bool fading = false;
	bool pressed = false;
	std::weak_ptr<mpc::hardware::HwPad> pad;
	int padhitBrightness = 0;
	int getVelo(int x, int y);
	mpc::Mpc& mpc;
	Image padhitImg;
	Rectangle <float> rect;

public:
	void paint(Graphics& g) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void timerCallback() override;
	bool isInterestedInFileDrag(const StringArray& files);
	void filesDropped(const StringArray& files, int x, int y);

public:
	void update(moduru::observer::Observable* o, nonstd::any arg) override;
	void setBounds();

public:
	PadControl(mpc::Mpc& mpc, Rectangle<float> rect, std::weak_ptr<mpc::hardware::HwPad> pad, Image padhit, const String& componentName);
	~PadControl();
};
