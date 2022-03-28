#pragma once
#include "juce_graphics/juce_graphics.h"

class Constants {

public:
	static const float TFACTOR;
	static const int KBLABEL_FONT_SIZE = 20;
	static const int KBLABEL_OUTLINE_SIZE = 4;
	static const juce::Colour LCD_ON;
	static const juce::Colour LCD_HALF_ON;
	static const juce::Colour LCD_OFF;

	static juce::Rectangle<float>& RECKNOB_RECT();
	static juce::Rectangle<float>& VOLKNOB_RECT();
	static juce::Rectangle<float>& SLIDER_RECT();
	static juce::Rectangle<float>& LCD_RECT();
	static juce::Rectangle<float>& DATAWHEEL_RECT();

private:
	static juce::Rectangle<float> RECKNOB_RECT_;
	static juce::Rectangle<float> VOLKNOB_RECT_;
	static juce::Rectangle<float> SLIDER_RECT_;
	static juce::Rectangle<float> LCD_RECT_;
	static juce::Rectangle<float> DATAWHEEL_RECT_;

};
