#pragma once
#include "VmpcComponent.hpp"

#include <Observer.hpp>

#include <memory>

namespace vmpc_juce::gui { class AuxLCDWindow; }

namespace mpc { class Mpc; }

namespace mpc::lcdgui {
	class LayeredScreen;
}

namespace vmpc_juce::gui::bitmap {
class LCDControl
	: public VmpcComponent
	, public juce::Timer
    , public mpc::Observer
{

private:
    vmpc_juce::gui::AuxLCDWindow* auxWindow = nullptr;
    mpc::Mpc& mpc;
	std::shared_ptr<mpc::lcdgui::LayeredScreen> ls;
	juce::Image lcd;
    juce::Rectangle<int> dirtyRect;
    std::function<void()> resetAuxWindowF;
    std::function<void()> resetKeyboardAuxParent;
    std::function<juce::Image&()> getLcdImage;

public:
    void resetAuxWindow();
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
    LCDControl(mpc::Mpc& mpc);
    ~LCDControl() override;

    void update(mpc::Observable* o, mpc::Message) override;
};
} // namespace vmpc_juce::gui::bitmap
