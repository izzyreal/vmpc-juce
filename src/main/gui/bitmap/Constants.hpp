#pragma once
#include "juce_graphics/juce_graphics.h"

namespace vmpc_juce::gui::bitmap {
class Constants {

public:
	static const juce::Colour LCD_ON;
	static const juce::Colour LCD_HALF_ON;
	static const juce::Colour LCD_OFF;

	static juce::Rectangle<int>& recKnobRect();
	static juce::Rectangle<int>& volKnobRect();
	static juce::Rectangle<int>& sliderRect();
	static juce::Rectangle<int>& lcdRect();
	static juce::Rectangle<int>& dataWheelRect();
};
} // namespace vmpc_juce::gui::bitmap
