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
	, public juce::Timer
	, public juce::FileDragAndDropTarget
	, public moduru::observer::Observer
{

private:
    mpc::Mpc& mpc;
    std::weak_ptr<mpc::hardware::HwPad> pad;
    juce::Image padhitImg;
    juce::Rectangle <float> rect;

    bool fading = false;
	bool pressed = false;
	int padhitBrightness = 0;
	int getVelo(int x, int y);

public:
	void paint(juce::Graphics& g) override;
	void mouseDown(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;
	void mouseDoubleClick(const juce::MouseEvent& event) override;
	void timerCallback() override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

public:
	void update(moduru::observer::Observable* o, nonstd::any arg) override;
	void setBounds();

public:
	PadControl(mpc::Mpc& mpc, juce::Rectangle<float> rect, std::weak_ptr<mpc::hardware::HwPad> pad, juce::Image padhit);
	~PadControl() override;
};
