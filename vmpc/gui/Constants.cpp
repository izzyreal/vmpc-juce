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

const Colour Constants::LCD_ON = Colour::fromRGBA(86, 61, 145, 255);
const Colour Constants::LCD_OFF = Colour::fromRGBA(170, 248, 218, 255);
const Colour Constants::LCD_HALF_ON = Colour::fromRGBA(128, 154, 181, 255);

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


/*
Rectangle<float>* Constants::PLUG_RECT()
{
    return &PLUG_RECT_;
}
Rectangle<float> Constants::PLUG_RECT_ = Rectangle<float>(0, 0, GUI_WIDTH, GUI_HEIGHT);

IColor* Constants::TRANS_BLACK() {
    return &TRANS_BLACK_;
}
IColor Constants::TRANS_BLACK_ = IColor(0, 0, 0, 0);

IColor* Constants::LCD_HALF_ON()
{
	return &LCD_HALF_ON_;
}
IColor Constants::LCD_HALF_ON_ = IColor(255, 128, 154, 181);

IColor* Constants::LCD_OFF()
{
	return &LCD_OFF_;
}
IColor Constants::LCD_OFF_ = IColor(255, 170, 248, 218);
*/