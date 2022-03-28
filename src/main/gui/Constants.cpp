#include "Constants.h"

//const float Constants::TFACTOR = .25f;
const float Constants::TFACTOR = .05f;

juce::Rectangle<float>& Constants::DATAWHEEL_RECT()
{
	return DATAWHEEL_RECT_;
}
juce::Rectangle<float> Constants::DATAWHEEL_RECT_ = juce::Rectangle<float>(378, 415, 171, 171);

juce::Rectangle<float>& Constants::LCD_RECT()
{
    return LCD_RECT_;
}
juce::Rectangle<float> Constants::LCD_RECT_ = juce::Rectangle<float>(116, 102, 496, 120);

const juce::Colour Constants::LCD_ON = juce::Colour::fromRGB(86, 61, 145);
const juce::Colour Constants::LCD_OFF = juce::Colour::fromRGB(170, 248, 218);
const juce::Colour Constants::LCD_HALF_ON = juce::Colour::fromRGB(128, 154, 181);

juce::Rectangle<float>& Constants::RECKNOB_RECT()
{
	return RECKNOB_RECT_;
}
juce::Rectangle<float> Constants::RECKNOB_RECT_ = juce::Rectangle<float>(1014, 183, 72, 73);

juce::Rectangle<float>& Constants::VOLKNOB_RECT()
{
	return VOLKNOB_RECT_;
}
juce::Rectangle<float> Constants::VOLKNOB_RECT_ = juce::Rectangle<float>(1137, 181, 74, 75);

juce::Rectangle<float>& Constants::SLIDER_RECT()
{
	return SLIDER_RECT_;

}
juce::Rectangle<float> Constants::SLIDER_RECT_ = juce::Rectangle<float>(33, 668, 128, 247);
