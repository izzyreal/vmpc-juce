#pragma once
#include <Fl/Fl_Input.H>
#include <Mpc.hpp>

class KeyboardHandler : Fl_Input
{
public:
	KeyboardHandler(mpc::Mpc& _mpcRef);
private:
	mpc& _mpcRef;
};

