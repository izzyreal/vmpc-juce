#include "Constants.h"

//const float Constants::TFACTOR = .25f;
const float Constants::TFACTOR = .05f;

Rectangle<float>* Constants::DATAWHEEL_RECT()
{
	return &DATAWHEEL_RECT_;
}
Rectangle<float> Constants::DATAWHEEL_RECT_ = Rectangle<float>(378, 415, 171, 171);

Rectangle<float> * Constants::LCD_RECT()
{
    return &LCD_RECT_;
}
Rectangle<float> Constants::LCD_RECT_ = Rectangle<float>(116, 102, 496, 120);

const Colour Constants::LCD_ON = Colour::fromRGB(86, 61, 145);
const Colour Constants::LCD_OFF = Colour::fromRGB(170, 248, 218);
const Colour Constants::LCD_HALF_ON = Colour::fromRGB(128, 154, 181);

Rectangle<float>* Constants::RECKNOB_RECT()
{
	return &RECKNOB_RECT_;
}
Rectangle<float> Constants::RECKNOB_RECT_ = Rectangle<float>(1014, 183, 72, 73);

Rectangle<float>* Constants::VOLKNOB_RECT()
{
	return &VOLKNOB_RECT_;
}
Rectangle<float> Constants::VOLKNOB_RECT_ = Rectangle<float>(1137, 181, 74, 75);

Rectangle<float>* Constants::SLIDER_RECT()
{
	return &SLIDER_RECT_;

}
Rectangle<float> Constants::SLIDER_RECT_ = Rectangle<float>(33, 668, 128, 247);
