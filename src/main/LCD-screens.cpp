#include "mpc-screens.h"

#include <lcdgui/LayeredScreen.hpp>
#include <lcdgui/Layer.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/screens/OthersScreen.hpp>

#include <Fl/Fl.H>
#include <iostream>
//#include <FL/x.H>
#include <FL/Enumerations.H>
//#include <FL/Fl_Window.H>
//#include <FL/Fl_Device.H>
//#include <FL/Fl_Box.H>
//#include <FL/fl_draw.H>
#include <gui/BasicStructs.hpp>

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;

LCDControl::LCDControl(mpc::Mpc& _mpc, std::weak_ptr<mpc::lcdgui::LayeredScreen> _ls) : mpc(_mpc), ls(_ls)
{
	Fl_Box lcd = Fl_Box(10, 10, 248, 60);
	auto othersScreen = mpc.screens->get<OthersScreen>("others");
	othersScreen->addObserver(this);
}

void LCDControl::update(moduru::observer::Observable*, nonstd::any msg)
{
	auto message = nonstd::any_cast<std::string>(msg);

	if (message.compare("contrast") == 0)
	{
		ls.lock()->getFocusedLayer().lock()->SetDirty(); // Could be done less invasively by just redrawing the current pixels of the LCD screens, but with updated colors
		//repaint();
	}
}

void LCDControl::drawPixelsToImg()
{
	auto pixels = ls.lock()->getPixels();
	for (int y = 0; y < 60; y++) {
		for (int x = 0; x < 248; x++) {
			if ((*pixels)[x][y]) {
				fl_color(FL_BLUE);
			}
			else {
				fl_color(FL_WHITE);
			}
			fl_point(x, y);
		}
	}
}

void LCDControl::checkLsDirty()
{
	if (ls.lock()->IsDirty())
	{
		auto dirtyArea = ls.lock()->getDirtyArea();
		Fl_Box dirtyRect = Fl_Box(dirtyArea.L, dirtyArea.T, dirtyArea.W(), dirtyArea.H());
		ls.lock()->Draw();
		drawPixelsToImg();
		//std::cout << "test" << std::endl;
	}
}