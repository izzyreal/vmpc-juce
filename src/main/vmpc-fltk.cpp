#define WIN32
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <Fl/Fl_Image.H>
#include <FL/fl_draw.H>
#include <Fl/Fl_Shared_Image.H>
#include <FL/Fl_BMP_Image.H>
#include <Fl/Fl_RGB_Image.H>
#include <Fl/Fl_PNG_Image.H>

#include <Mpc.hpp>
//#include "mpc-screens.h"

#include <lcdgui/LayeredScreen.hpp>
#include <lcdgui/Layer.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/screens/OthersScreen.hpp>

//#include <gui/BasicStructs.hpp>
#include <iostream>

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;
using namespace mpc;

void drawScreen(void* mpcPtr) {
	mpc::Mpc* mpc = (mpc::Mpc*) mpcPtr;
	puts("TICK");
	auto pixels = mpc->getLayeredScreen().lock()->getPixels();
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
	//std::cout << "test loop" << std::endl;
	Fl::repeat_timeout(1.0, drawScreen, &mpc);
}

int main(int argc, char** argv) {
	Mpc mpc;
	mpc.init(44100, 1, 1);
	mpc.getLayeredScreen().lock()->openScreen("sequencer");
	Fl_Window* window = new Fl_Window(1000, 1000);
	Fl_Box* box = new Fl_Box(10, 10, 250, 70);
	//window->fullscreen();

	/*LCDControl* display;
	display = new LCDControl(mpc, mpc.getLayeredScreen());
	display->drawPixelsToImg();*/

	window->end();
	window->show(argc, argv);
	//Fl::add_timeout(1.0, initialise, &mpc);
	Fl::add_timeout(1.0, drawScreen, &mpc);
	return Fl::run();
}