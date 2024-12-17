/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
#include "LedControl.hpp"

#include <Mpc.hpp>

#include <controls/Controls.hpp>

#include <sequencer/Sequencer.hpp>

#include <string>

using namespace vmpc_juce::gui::bitmap;

LedControl::LedControl(mpc::Mpc& _mpc, juce::Image& _ledGreen, juce::Image& _ledRed)
: mpc (_mpc), ledGreen (_ledGreen), ledRed (_ledRed)
{
	int x, y;
	int ledSize = 10;

	x = 874;
	y = 216;

	sixteenLevels = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 791;
	y = 298;
	nextSeq = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 875;
	y = 298;
	trackMute = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 103;
	y = 601;
	after = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 226;
	y = 686;
	undoSeq = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 214;
	y = 833;
	rec = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 294;
	y = 833;
	overDub = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 451;
	y = 830;
	play = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 791;
	y = 216;
	fullLevel = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 958;
	y = 298;
	padBankA = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 1041;
	y = 298;
	padBankB = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 1124;
	y = 297;
	padBankC = juce::Rectangle<float>(x, y, ledSize, ledSize);
	x = 1206;
	y = 296;
	padBankD = juce::Rectangle<float>(x, y, ledSize, ledSize);

	fullLevelLed = new Led(ledGreen, fullLevel);
	sixteenLevelsLed = new Led(ledGreen, sixteenLevels);
	nextSeqLed = new Led(ledGreen, nextSeq);
	trackMuteLed = new Led(ledGreen, trackMute);
	padBankALed = new Led(ledGreen, padBankA);
	padBankBLed = new Led(ledGreen, padBankB);
	padBankCLed = new Led(ledGreen, padBankC);
	padBankDLed = new Led(ledGreen, padBankD);
	afterLed = new Led(ledGreen, after);
	undoSeqLed = new Led(ledRed, undoSeq);
	recLed = new Led(ledRed, rec);
	overDubLed = new Led(ledRed, overDub);
	playLed = new Led(ledGreen, play);
}

void LedControl::addAndMakeVisible(juce::Component* parent) {
	parent->addAndMakeVisible(padBankALed);
	parent->addAndMakeVisible(fullLevelLed);
	parent->addAndMakeVisible(sixteenLevelsLed);
	parent->addAndMakeVisible(nextSeqLed);
	parent->addAndMakeVisible(trackMuteLed);
	parent->addAndMakeVisible(padBankALed);
	parent->addAndMakeVisible(padBankBLed);
	parent->addAndMakeVisible(padBankCLed);
	parent->addAndMakeVisible(padBankDLed);
	parent->addAndMakeVisible(afterLed);
	parent->addAndMakeVisible(undoSeqLed);
	parent->addAndMakeVisible(recLed);
	parent->addAndMakeVisible(overDubLed);
	parent->addAndMakeVisible(playLed);
}

void LedControl::setTransform(juce::AffineTransform transform) {
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

void LedControl::timerCallback()
{
    auto seq = mpc.getSequencer();
    auto controls = mpc.getControls();
    auto stepEditor = mpc.getLayeredScreen()->getCurrentScreenName() == "step-editor";

    setUndoSeq(seq->isUndoSeqAvailable());

    setPlay(seq->isPlaying());

    auto isPlayingButNotOverdubbingAndOverdubIsPressed = seq->isPlaying() && !seq->isOverDubbing() && controls->isOverDubPressed();

    if (isPlayingButNotOverdubbingAndOverdubIsPressed)
    {
        setOverDub(false);
    }
    else
    {
        setOverDub(controls->isOverDubPressed() || seq->isOverDubbing() || stepEditor);
    }

    auto isPlayingButNotRecordingAndRecIsPressed = seq->isPlaying() && !seq->isRecording() && controls->isRecPressed();

    if (isPlayingButNotRecordingAndRecIsPressed)
    {
        setRec(false);
    } else {
        setRec(controls->isRecPressed() || seq->isRecording());
    }
}

void LedControl::update(mpc::Observable*, mpc::Message message)
{
    const auto msg = std::get<std::string>(message);

	if (msg == "full-level-on") {
		setFullLevel(true);
	}
	else if(msg == "full-level-off") {
		setFullLevel(false);
	}
	else if (msg == "sixteen-levels-on") {
		setSixteenLevels(true);
	}
	else if (msg == "sixteen-levels-off") {
		setSixteenLevels(false);
	}
	else if (msg == "next-seq-on") {
		setNextSeq(true);
	}
	else if (msg == "next-seq-off") {
		setNextSeq(false);
	}
	else if (msg == "track-mute-on") {
		setTrackMute(true);
	}
	else if (msg == "track-mute-off") {
		setTrackMute(false);
	}
	else if (msg == "pad-bank-a-on") {
		setPadBankA(true);
	}
	else if (msg == "pad-bank-a-off") {
		setPadBankA(false);
	}
	else if (msg == "pad-bank-b-on") {
		setPadBankB(true);
	}
	else if (msg == "pad-bank-b-off") {
		setPadBankB(false);
	}
	else if (msg == "pad-bank-c-on") {
		setPadBankC(true);
	}
	else if (msg == "pad-bank-c-off") {
		setPadBankC(false);
	}
	else if (msg == "pad-bank-d-on") {
		setPadBankD(true);
	}
	else if (msg == "pad-bank-d-off") {
		setPadBankD(false);
	}
	else if (msg == "after-on") {
		setAfter(true);
	}
	else if (msg == "after-off") {
		setAfter(false);
	}
	else if (msg == "undo-seq-on") {
		setUndoSeq(true);
	}
	else if (msg == "undo-seq-off") {
		setUndoSeq(false);
	}
	else if (msg == "rec-on") {
		setRec(true);
	}
	else if (msg == "rec-off") {
		setRec(false);
	}
	else if (msg == "overdub-on") {
		setOverDub(true);
	}
	else if (msg == "overdub-off") {
		setOverDub(false);
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
