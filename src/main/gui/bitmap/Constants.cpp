#include "Constants.hpp"

using namespace vmpc_juce::gui::bitmap;

juce::Rectangle<int>& Constants::dataWheelRect()
{
    static auto rect = juce::Rectangle<int>(378, 415, 171, 171);
	return rect;
}

juce::Rectangle<int>& Constants::lcdRect()
{
    static auto rect = juce::Rectangle<int>(116, 102, 496, 120);
    return rect;
}

const juce::Colour Constants::LCD_ON = juce::Colour::fromRGB(86, 61, 145);
const juce::Colour Constants::LCD_OFF = juce::Colour::fromRGB(170, 248, 218);
const juce::Colour Constants::LCD_HALF_ON = juce::Colour::fromRGB(128, 154, 181);

juce::Rectangle<int>& Constants::recKnobRect()
{
    static auto rect = juce::Rectangle<int>(1014, 183, 72, 73);
	return rect;
}

juce::Rectangle<int>& Constants::volKnobRect()
{
    static auto rect = juce::Rectangle<int>(1137, 181, 74, 75);
	return rect;
}

juce::Rectangle<int>& Constants::sliderRect()
{
    static auto rect = juce::Rectangle<int>(33, 668, 128, 247);
	return rect;
}
