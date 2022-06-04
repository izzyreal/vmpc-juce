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
#include <Fl/Fl_Input.H>
#include "KeyboardHandler.h"

#include <Mpc.hpp>
//#include "mpc-screens.h"

#include <lcdgui/LayeredScreen.hpp>
#include <lcdgui/Layer.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/screens/OthersScreen.hpp>

//#include <gui/BasicStructs.hpp>
#include <iostream>
#include <controls/KeyEvent.cpp>

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;
using namespace mpc;

Fl_Widget* inputWidget;

void drawScreen(void* mpcPtr) {
	mpc::Mpc* mpc = (mpc::Mpc*)mpcPtr;
	auto ls = mpc->getLayeredScreen().lock();
	mpc->getLayeredScreen().lock()->Draw();
	auto pixels = mpc->getLayeredScreen().lock()->getPixels();
	for (int y = 0; y < 60; y++) {
		for (int x = 0; x < 248; x++) {
			if ((*pixels)[x][y]) {
				fl_color(FL_BLACK);
			}
			else {
				fl_color(234, 243, 219);
			}
			fl_point(x, y);
		}
	}
	std::cout << "test loop" << std::endl;
	Fl::repeat_timeout(0.05, drawScreen, mpc);
}

int rawHandler(void* event, void* mpcPtr)
{
	mpc::Mpc* mpc = (mpc::Mpc*)mpcPtr;
	auto keyEventHandler = mpc->getControls().lock()->getKeyEventHandler().lock();
	auto eventMsg = (MSG*)event;
	switch (eventMsg->message)
	{
	case WM_KEYDOWN:
		keyEventHandler->handle(KeyEvent(eventMsg->wParam, true));
	}
	return 0;
}

int escKeyConsumer(int event)
{
	if (event == FL_SHORTCUT) {
		return 1;
	}
	return 0;
}

int main(int argc, char** argv) {
	Mpc mpc;
	mpc.init(44100, 1, 1);

	Fl_Window* window = new Fl_Window(800, 600);
	Fl_Box* box = new Fl_Box(10, 10, 250, 70);
	//window->fullscreen();

	window->end();
	window->show(argc, argv);
	Fl::add_timeout(1.0, drawScreen, &mpc);
	Fl::add_handler(escKeyConsumer);
	Fl::add_system_handler(rawHandler, &mpc);
	return Fl::run();
}