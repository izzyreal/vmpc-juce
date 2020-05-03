/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

#include "gui/DataWheelControl.h"
#include "gui/LCDControl.h"
#include "gui/InputCatcherControl.h"
#include "gui/SliderControl.hpp"
#include "gui/PadControl.hpp"
#include "gui/LedControl.hpp"
#include "gui/KnobControl.hpp"
#include "gui/ButtonControl.hpp"

//==============================================================================
/**
*/
class VmpcAudioProcessorEditor  : public AudioProcessorEditor
{

public:
    VmpcAudioProcessorEditor (VmpcAudioProcessor&);
    ~VmpcAudioProcessorEditor();

    void paint (Graphics&) override;
    void resized() override;

private:
	void initialise();

private:
	Component::SafePointer<SplashScreen> mpcSplashScreen;
	Label versionLabel;
    VmpcAudioProcessor& processor;
	DataWheelControl* dataWheel;
	KnobControl* recKnob;
	KnobControl* volKnob;
	SliderControl* slider;
	LCDControl* lcd;
	LedControl* leds;
	InputCatcherControl* inputCatcher;
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

	bool initialFocusSet = false;

	VmpcAudioProcessor& getProcessor() const
	{
		return static_cast<VmpcAudioProcessor&> (processor);
	}
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcAudioProcessorEditor)
};
