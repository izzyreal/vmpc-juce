#include "LedControl.hpp"

#include <string>

using namespace std;

LedControl::LedControl(Image ledGreen, Image ledRed, InputCatcherControl* ipc)
{
	this->ledGreen = ledGreen;
	this->ledRed = ledRed;
	this->ipc = ipc;

	int x, y;
	int ledSize = 10;

	x = 874;
	y = 216;

	sixteenLevels = Rectangle<float>(x, y, ledSize, ledSize);
	x = 791;
	y = 298;
	nextSeq = Rectangle<float>(x, y, ledSize, ledSize);
	x = 875;
	y = 298;
	trackMute = Rectangle<float>(x, y, ledSize, ledSize);
	x = 103;
	y = 601;
	after = Rectangle<float>(x, y, ledSize, ledSize);
	x = 226;
	y = 686;
	undoSeq = Rectangle<float>(x, y, ledSize, ledSize);
	x = 214;
	y = 833;
	rec = Rectangle<float>(x, y, ledSize, ledSize);
	x = 294;
	y = 833;
	overDub = Rectangle<float>(x, y, ledSize, ledSize);
	x = 451;
	y = 830;
	play = Rectangle<float>(x, y, ledSize, ledSize);
	x = 791;
	y = 216;
	fullLevel = Rectangle<float>(x, y, ledSize, ledSize);
	x = 958;
	y = 298;
	padBankA = Rectangle<float>(x, y, ledSize, ledSize);
	x = 1041;
	y = 298;
	padBankB = Rectangle<float>(x, y, ledSize, ledSize);
	x = 1124;
	y = 297;
	padBankC = Rectangle<float>(x, y, ledSize, ledSize);
	x = 1206;
	y = 296;
	padBankD = Rectangle<float>(x, y, ledSize, ledSize);

	fullLevelLed = new Led(ledGreen, fullLevel);
	fullLevelLed->setInputCatcher(ipc);
	sixteenLevelsLed = new Led(ledGreen, sixteenLevels);
	sixteenLevelsLed->setInputCatcher(ipc);
	nextSeqLed = new Led(ledGreen, nextSeq);
	nextSeqLed->setInputCatcher(ipc);
	trackMuteLed = new Led(ledGreen, trackMute);
	trackMuteLed->setInputCatcher(ipc);
	padBankALed = new Led(ledGreen, padBankA);
	padBankALed->setInputCatcher(ipc);
	padBankBLed = new Led(ledGreen, padBankB);
	padBankBLed->setInputCatcher(ipc);
	padBankCLed = new Led(ledGreen, padBankC);
	padBankCLed->setInputCatcher(ipc);
	padBankDLed = new Led(ledGreen, padBankD);
	padBankDLed->setInputCatcher(ipc);
	afterLed = new Led(ledGreen, after);
	afterLed->setInputCatcher(ipc);
	undoSeqLed = new Led(ledGreen, undoSeq);
	undoSeqLed->setInputCatcher(ipc);
	recLed = new Led(ledRed, rec);
	recLed->setInputCatcher(ipc);
	overDubLed = new Led(ledRed, overDub);
	overDubLed->setInputCatcher(ipc);
	playLed = new Led(ledGreen, play);	
	playLed->setInputCatcher(ipc);
}

void LedControl::addAndMakeVisible(AudioProcessorEditor* editor) {
	editor->addAndMakeVisible(padBankALed);
	editor->addAndMakeVisible(fullLevelLed);
	editor->addAndMakeVisible(sixteenLevelsLed);
	editor->addAndMakeVisible(nextSeqLed);
	editor->addAndMakeVisible(trackMuteLed);
	editor->addAndMakeVisible(padBankALed);
	editor->addAndMakeVisible(padBankBLed);
	editor->addAndMakeVisible(padBankCLed);
	editor->addAndMakeVisible(padBankDLed);
	editor->addAndMakeVisible(afterLed);
	editor->addAndMakeVisible(undoSeqLed);
	editor->addAndMakeVisible(recLed);
	editor->addAndMakeVisible(overDubLed);
	editor->addAndMakeVisible(playLed);
}

void LedControl::setTransform(AffineTransform transform) {
	fullLevelLed->setTransform(transform);
	sixteenLevelsLed->setTransform(transform);
	nextSeqLed->setTransform(transform);
	trackMuteLed->setTransform(transform);
	padBankALed->setTransform(transform);
	padBankBLed->setTransform(transform);
	padBankCLed->setTransform(transform);
	padBankDLed->setTransform(transform);
	afterLed->setTransform(transform);
	undoSeqLed->setTransform(transform);
	recLed->setTransform(transform);
	overDubLed->setTransform(transform);
	playLed->setTransform(transform);
}

void LedControl::setBounds() {
	fullLevelLed->setBounds();
	sixteenLevelsLed->setBounds();
	nextSeqLed->setBounds();
	trackMuteLed->setBounds();
	padBankALed->setBounds();
	padBankBLed->setBounds();
	padBankCLed->setBounds();
	padBankDLed->setBounds();
	afterLed->setBounds();
	undoSeqLed->setBounds();
	recLed->setBounds();
	overDubLed->setBounds();
	playLed->setBounds();
}


void LedControl::setPadBankA(bool b)
{
    padBankALed->setOn(b);
}

void LedControl::setPadBankB(bool b)
{
    padBankBLed->setOn(b);
}

void LedControl::setPadBankC(bool b)
{
    padBankCLed->setOn(b);
}

void LedControl::setPadBankD(bool b)
{
    padBankDLed->setOn(b);
}

void LedControl::setFullLevel(bool b)
{
    fullLevelLed->setOn(b);
}

void LedControl::setSixteenLevels(bool b)
{
    sixteenLevelsLed->setOn(b);
}

void LedControl::setNextSeq(bool b)
{
    nextSeqLed->setOn(b);
}

void LedControl::setTrackMute(bool b)
{
    trackMuteLed->setOn(b);
}

void LedControl::setAfter(bool b)
{
    afterLed->setOn(b);
}

void LedControl::setRec(bool b)
{
    recLed->setOn(b);
}

void LedControl::setOverDub(bool b)
{
    overDubLed->setOn(b);
}

void LedControl::setPlay(bool b)
{
    playLed->setOn(b);
}

void LedControl::setUndoSeq(bool b)
{
    undoSeqLed->setOn(b);
}

void LedControl::update(moduru::observer::Observable* o, boost::any arg) {
	string s = boost::any_cast<string>(arg);
	if (s.compare("fulllevelon") == 0) {
		setFullLevel(true);
	}
	else if(s.compare("fullleveloff") == 0) {
		setFullLevel(false);
	}
	else if (s.compare("sixteenlevelson") == 0) {
		setSixteenLevels(true);
	}
	else if (s.compare("sixteenlevelsoff") == 0) {
		setSixteenLevels(false);
	}
	else if (s.compare("nextseqon") == 0) {
		setNextSeq(true);
	}
	else if (s.compare("nextseqoff") == 0) {
		setNextSeq(false);
	}
	else if (s.compare("trackmuteon") == 0) {
		setTrackMute(true);
	}
	else if (s.compare("trackmuteoff") == 0) {
		setTrackMute(false);
	}
	else if (s.compare("padbankaon") == 0) {
		setPadBankA(true);
	}
	else if (s.compare("padbankaoff") == 0) {
		setPadBankA(false);
	}
	else if (s.compare("padbankbon") == 0) {
		setPadBankB(true);
	}
	else if (s.compare("padbankboff") == 0) {
		setPadBankB(false);
	}
	else if (s.compare("padbankcon") == 0) {
		setPadBankC(true);
	}
	else if (s.compare("padbankcoff") == 0) {
		setPadBankC(false);
	}
	else if (s.compare("padbankdon") == 0) {
		setPadBankD(true);
	}
	else if (s.compare("padbankdoff") == 0) {
		setPadBankD(false);
	}
	else if (s.compare("afteron") == 0) {
		setAfter(true);
	}
	else if (s.compare("afteroff") == 0) {
		setAfter(false);
	}
	else if (s.compare("undoseqon") == 0) {
		setUndoSeq(true);
	}
	else if (s.compare("undoseqoff") == 0) {
		setUndoSeq(false);
	}
	else if (s.compare("recon") == 0) {
		setRec(true);
	}
	else if (s.compare("recoff") == 0) {
		setRec(false);
	}
	else if (s.compare("overdubon") == 0) {
		setOverDub(true);
	}
	else if (s.compare("overduboff") == 0) {
		setOverDub(false);
	}
	else if (s.compare("playon") == 0) {
		setPlay(true);
	}
	else if (s.compare("playoff") == 0) {
		setPlay(false);
	}
}

LedControl::~LedControl() {
	delete fullLevelLed;
	delete sixteenLevelsLed;
	delete nextSeqLed;
	delete trackMuteLed;
	delete padBankALed;
	delete padBankBLed;
	delete padBankCLed;
	delete padBankDLed;
	delete afterLed;
	delete undoSeqLed;
	delete recLed;
	delete overDubLed;
	delete playLed;
}
