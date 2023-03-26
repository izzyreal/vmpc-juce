#pragma once
#include "VmpcComponent.hpp"

#include <observer/Observer.hpp>

#include <vector>
#include <memory>

class Keyboard;
class AuxLCD;

namespace mpc { class Mpc; }

namespace mpc::lcdgui {
	class LayeredScreen;
}

class LCDControl
	: public VmpcComponent
	, public juce::Timer
	, public moduru::observer::Observer
{

private:
    juce::ResizableWindow* auxWindow = nullptr;
    AuxLCD* auxLcd = nullptr;
    mpc::Mpc& mpc;
	std::shared_ptr<mpc::lcdgui::LayeredScreen> ls;
	juce::Image lcd;
    juce::Rectangle<int> dirtyRect;

public:
    void resetAuxWindow() { if (auxWindow != nullptr) { auxWindow->removeFromDesktop(); delete auxWindow; auxWindow = nullptr;}}
    void checkLsDirty();
	void drawPixelsToImg();
	void paint(juce::Graphics& g) override;
	void timerCallback() override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
  void mouseDown(const juce::MouseEvent& e) override {
    getParentComponent()->mouseDown(e);
  }
  void mouseDrag(const juce::MouseEvent& e) override {
    getParentComponent()->mouseDrag(e);
  }

public:
	LCDControl(mpc::Mpc& mpc, Keyboard*);
    ~LCDControl() override;

    void update(moduru::observer::Observable* o, nonstd::any msg) override;

private:
    friend class AuxLCD;
};
