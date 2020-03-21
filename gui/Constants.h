#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

class Constants {

public:
	static const float TFACTOR;
	static const int KBLABEL_FONT_SIZE = 20;
	static const int KBLABEL_OUTLINE_SIZE = 4;
	static const Colour LCD_ON;
	static const Colour LCD_HALF_ON;
	static const Colour LCD_OFF;

	static Rectangle<float>* RECKNOB_RECT();
	static Rectangle<float>* VOLKNOB_RECT();
	static Rectangle<float>* SLIDER_RECT();
	static Rectangle<float>* LCD_RECT();
	static Rectangle<float>* DATAWHEEL_RECT();

private:
	static Rectangle<float> RECKNOB_RECT_;
	static Rectangle<float> VOLKNOB_RECT_;
	static Rectangle<float> SLIDER_RECT_;
	static Rectangle<float> LCD_RECT_;
	static Rectangle<float> DATAWHEEL_RECT_;

};
