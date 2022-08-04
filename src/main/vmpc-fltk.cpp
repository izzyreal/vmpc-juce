#include <controls/Controls.hpp>
#include <controls/GlobalReleaseControls.hpp>
#include <controls/KeyEventHandler.hpp>
#include <controls/KeyEvent.hpp>

#include "portaudio.h"

#include <Mpc.hpp>
#include <audiomidi/AudioMidiServices.hpp>
#include <audio/server/NonRealTimeAudioServer.hpp>

#include <lcdgui/LayeredScreen.hpp>
#include <lcdgui/Layer.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/screens/OthersScreen.hpp>

#include <iostream>
#include <controls/KeyEvent.cpp>

#if _WIN32
#define WIN32
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>
#include <stdio.h>
#include <time.h>
#include <FL/fl_draw.H>

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;
using namespace mpc;

#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (512)

#define XSIZE 249
#define YSIZE 60
#define UPDATE_RATE 0.2

static int patestCallback(const void* inputBuffer, void* outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* mpcPtr)
{
	mpc::Mpc* mpc = (mpc::Mpc*)mpcPtr;
	float* out = (float*)outputBuffer;
	unsigned long i;

	(void)timeInfo; /* Prevent unused variable warnings. */
	(void)statusFlags;
	(void)inputBuffer;

	std::vector<std::vector<float>> tempBufferOut(2, std::vector<float>(FRAMES_PER_BUFFER));
	std::vector<float*> tempBufferOutAdapter{ tempBufferOut[0].data(), tempBufferOut[1].data() };

	auto server = mpc->getAudioMidiServices().lock()->getAudioServer();
	server->work(nullptr, tempBufferOutAdapter.data(), FRAMES_PER_BUFFER, 0, 2);

	for (i = 0; i < framesPerBuffer; i++)
	{
		*out++ = tempBufferOut[0][i];
		*out++ = tempBufferOut[1][i];
	}

	return paContinue;
}

//void drawScreen(void* mpcPtr) {
//	mpc::Mpc* mpc = (mpc::Mpc*)mpcPtr;
//	auto ls = mpc->getLayeredScreen().lock();
//	mpc->getLayeredScreen().lock()->Draw();
//	auto pixels = mpc->getLayeredScreen().lock()->getPixels();
//	for (unsigned int y = 0; y < 60; y++) {
//		for (unsigned int x = 0; x < 248; x++) {
//			if ((*pixels)[x][y]) {
//				fl_color(FL_BLACK);
//			}
//			else {
//				fl_color(FL_GREEN);
//			}
//			fl_point(x, y);
//		}
//	}
//	//std::cout << "test loop" << std::endl;
//	Fl::repeat_timeout(0.2, drawScreen, mpc);
//}

int rawHandler(void* event, void* mpcPtr)
{
	mpc::Mpc* mpc = (mpc::Mpc*)mpcPtr;
	auto keyEventHandler = mpc->getControls().lock()->getKeyEventHandler().lock();
#ifdef __linux__ 
	XEvent* xEvent = (XEvent*)event;
	if (xEvent->type == KeyPress) {
		XKeyEvent* xKeyEvent = (XKeyEvent*)xEvent; 
		uint32_t keyEventCode = xEvent->xkey.keycode;
		std::cout << keyEventCode << '\n';
		KeySym ks = XKeycodeToKeysym(fl_display, keyEventCode, 0);
		keyEventHandler->handle(KeyEvent(ks, true));
	} else if (xEvent->type == KeyRelease) {
		XKeyEvent* xKeyEvent = (XKeyEvent*)xEvent;
		uint32_t keyEventCode = xEvent->xkey.keycode;
		std::cout << keyEventCode << '\n';
		KeySym ks = XKeycodeToKeysym(fl_display, keyEventCode, 0);
		keyEventHandler->handle(KeyEvent(ks, false));
	}

#elif _WIN32
	auto eventMsg = (MSG*)event;
	switch (eventMsg->message)
	{
	case WM_KEYDOWN:
		keyEventHandler->handle(KeyEvent(eventMsg->wParam, true));
		break;
	case WM_KEYUP:
		keyEventHandler->handle(KeyEvent(eventMsg->wParam, false));
		break;
	}
#endif
	return 0;
}

int escKeyConsumer(int event)
{
	if (event == FL_SHORTCUT) {
		return 1;
	} 
	return 0;
}

void initialisePortAudio(mpc::Mpc *mpc)
{
	PaStreamParameters outputParameters;
	PaStream* stream;
	PaError err;

	err = Pa_Initialize();

	outputParameters.device = Pa_GetDefaultOutputDevice();
	if (outputParameters.device == paNoDevice) {
		fprintf(stderr, "Error: No default output device.\n");
		return;
	}
	outputParameters.channelCount = 2;   
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	err = Pa_OpenStream(
		&stream,
		NULL, 
		&outputParameters,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff,     
		patestCallback,
		mpc);
	
	err = Pa_StartStream(stream);

	printf("o");
}

class mpcWindow : public Fl_Double_Window {
	unsigned char pixbuf[YSIZE][XSIZE]; // image buffer
	
	// FLTK DRAW METHOD
	void draw() {
		fl_draw_image_mono((const uchar*)&pixbuf, 0, 0, XSIZE, YSIZE, 3, XSIZE * 3);
	}

	// TIMER CALLBACK: CALLED TO UPDATE THE DRAWING
	static void RenderImage_CB(void* userdata) {
		mpcWindow* win = (mpcWindow*)userdata;
		win->RenderImage();
		Fl::repeat_timeout(UPDATE_RATE, RenderImage_CB, userdata);
	}

public:
	// CTOR
	mpcWindow(int w, int h, const char* name = 0) : Fl_Double_Window(w, h, name) {
		end();
		RenderImage();                   // show first drawing
		// Start timer updating
		Fl::add_timeout(UPDATE_RATE, RenderImage_CB, (void*)this);
	}

	void PlotPixel(int x, int y) {
		
	}

	void RenderImage() {
		for (int x = 0; x < XSIZE; x++)
			for (int y = 0; y < YSIZE; y++)
				PlotPixel(x, y);
		redraw();
	}
};

int main(int argc, char** argv) {
	Fl::visual(FL_RGB);
	Mpc mpc;
	mpc.init(44100, 1, 1);
	auto server = mpc.getAudioMidiServices().lock()->getAudioServer();
	server->resizeBuffers(FRAMES_PER_BUFFER);
	Fl::set_color(FL_GREEN, 234, 243, 219);
	mpcWindow* win = new mpcWindow(XSIZE, YSIZE);
	//window->color(FL_GREEN);
	//Fl_Box* box = new Fl_Box(10, 10, 250, 70);
	//window->fullscreen();

	//Fl::add_timeout(0.2, drawScreen, &mpc);
	Fl::add_handler(escKeyConsumer);
	Fl::add_system_handler(rawHandler, &mpc);
	initialisePortAudio(&mpc);
	win->end();
	win->show(argc, argv);

	int exitCode = Fl::run();
	Pa_Terminate();
	return exitCode;
}