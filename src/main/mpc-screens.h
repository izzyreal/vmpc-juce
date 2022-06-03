#pragma once

#define WIN32
#include <Fl/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Bitmap.H>
#include <FL/fl_draw.H>
#include <observer/Observer.hpp>

#include <vector>
#include <memory>

namespace mpc { class Mpc; }

namespace mpc::lcdgui {
	class LayeredScreen;
}

class LCDControl
	 : public moduru::observer::Observer
{

private:
	mpc::Mpc& mpc;
	std::weak_ptr<mpc::lcdgui::LayeredScreen> ls;
	Fl_Box lcd();
	Fl_Box& dirtyRect = Fl_Box(0, 0, 1000, 1000);

public:
	void checkLsDirty();
	void drawPixelsToImg();
	//void paint(juce::Graphics& g) override;
	//void timerCallback() override;
	//void mouseDown(const juce::MouseEvent& e) override {
	//	getParentComponent()->mouseDown(e);
	//}
	//void mouseDrag(const juce::MouseEvent& e) override {
	//	getParentComponent()->mouseDrag(e);
	//}

public:
	LCDControl(mpc::Mpc& mpc, std::weak_ptr<mpc::lcdgui::LayeredScreen> ls);
	void update(moduru::observer::Observable* o, nonstd::any msg) override;

};
