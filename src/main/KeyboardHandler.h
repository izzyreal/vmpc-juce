#pragma once

#include <Fl/Fl.H>
#include <Fl/Fl_Input.H>
#include <iostream>

#include <Mpc.hpp>
#include <controls/Controls.hpp>
#include <controls/KeyEventHandler.hpp>
#include <controls/KeyEvent.hpp>

using namespace mpc::controls;

class KeyboardHandler : public Fl_Widget
{
public:
	KeyboardHandler(mpc::Mpc& _mpc) : Fl_Widget(0,0,0,0), mpc(_mpc) {}
	KeyboardHandler();

public:
	int handle(int event)
	{
		auto keyEventHandler = mpc.getControls().lock()->getKeyEventHandler().lock();

		if (event == FL_FOCUS)
			return 1;
		if (event == FL_UNFOCUS)
			return 1;

		if (event == FL_KEYBOARD)
		{
			if (Fl::event_original_key())
			{
				keyEventHandler->handle(KeyEvent(Fl::event_original_key(), true));
				std::cout << Fl::event_key() << std::endl;
				return 1;
			}
		}
		return 0;

	}
	virtual void draw() { ; }
private:
	mpc::Mpc &mpc;
};

