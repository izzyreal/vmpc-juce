#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "VmpcComponent.h"

class Led
	: public VmpcComponent
{

private:
	Image led;
	
	Rectangle<float> rect;
	bool on = false;

public:
	void setOn(bool b);
	void setBounds();

public:
	void paint(Graphics& g) override;

public:
	Led(Image led, Rectangle<float> rect);
	~Led();

};
