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
#include <FL/fl_draw.H>
#include <FL/Fl_Input.H>

#include <controls/Controls.hpp>
#include <controls/GlobalReleaseControls.hpp>
#include <controls/KeyEventHandler.hpp>
#include <controls/KeyEvent.hpp>

#include <stdio.h>
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

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;
using namespace mpc;

#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (512)

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
	//std::cout << "test loop" << std::endl;
	Fl::repeat_timeout(0.0001, drawScreen, mpc);
}

int rawHandler(void* event, void* mpcPtr)
{
	mpc::Mpc* mpc = (mpc::Mpc*)mpcPtr;
	auto keyEventHandler = mpc->getControls().lock()->getKeyEventHandler().lock();
#ifdef __linux__ 
	printf("Nothing");
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

int main(int argc, char** argv) {
	Mpc mpc;
	mpc.init(44100, 1, 1);
	auto server = mpc.getAudioMidiServices().lock()->getAudioServer();
	server->resizeBuffers(FRAMES_PER_BUFFER);
	Fl_Window* window = new Fl_Window(248, 60);
	Fl_Box* box = new Fl_Box(10, 10, 250, 70);
	//window->fullscreen();

	Fl::add_timeout(1.0, drawScreen, &mpc);
	Fl::add_handler(escKeyConsumer);
	Fl::add_system_handler(rawHandler, &mpc);
	initialisePortAudio(&mpc);

	window->end();
	window->show(argc, argv);

	int exitCode = Fl::run();
	Pa_Terminate();
	return exitCode;
}