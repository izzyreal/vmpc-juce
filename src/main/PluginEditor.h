/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "juce_gui_extra/juce_gui_extra.h"
#include "PluginProcessor.h"

#include "gui/DataWheelControl.h"
#include "gui/LCDControl.h"
#include "gui/KeyEventListener.h"
#include "gui/SliderControl.hpp"
#include "gui/PadControl.hpp"
#include "gui/LedControl.hpp"
#include "gui/KnobControl.hpp"
#include "gui/ButtonControl.hpp"

using namespace juce;

namespace mpc { class Mpc; }

class VmpcAudioProcessorEditor
	: public AudioProcessorEditor
{

public:
    VmpcAudioProcessorEditor(VmpcAudioProcessor&);
    ~VmpcAudioProcessorEditor();

    void paint (Graphics&) override;
    void resized() override;

private:
	void initialise();

private:
	mpc::Mpc& mpc;
	Component::SafePointer<SplashScreen> mpcSplashScreen;
	Label versionLabel;
    VmpcAudioProcessor& processor;
	DataWheelControl* dataWheel;
	KnobControl* recKnob;
	KnobControl* volKnob;
	SliderControl* slider;
	LCDControl* lcd;
	LedControl* leds;
	KeyEventListener* keyEventListener;
	std::vector<ButtonControl*> buttons;
	std::vector<PadControl*> pads;

	Image dataWheelImg;
	Image bgImg;
	Image sliderImg;
	Image recKnobImg;
	Image volKnobImg;
	Image padHitImg;
	Image ledRedImg;
	Image ledGreenImg;

	VmpcAudioProcessor& getProcessor() const
	{
		return static_cast<VmpcAudioProcessor&> (processor);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcAudioProcessorEditor)
};
