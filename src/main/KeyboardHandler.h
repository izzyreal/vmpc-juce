#pragma once
#include <Fl/Fl_Input.H>
#include <Mpc.hpp>

class KeyboardHandler : Fl_Input
{
	//mpc::Mpc& mpc;
	KeyboardHandler(mpc::Mpc& mpc);
};

