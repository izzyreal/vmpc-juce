/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "gui/Constants.h"

#include <hardware/Hardware.hpp>
#include <hardware/Led.hpp>
#include <audiomidi/AudioMidiServices.hpp>
#include <StartUp.hpp>

//==============================================================================
VmpcAudioProcessorEditor::VmpcAudioProcessorEditor (VmpcAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
	initialise();

	inputCatcher = new InputCatcherControl("inputcatcher");
	inputCatcher->setSize(1298, 994);
	inputCatcher->setWantsKeyboardFocus(true);
	addAndMakeVisible(inputCatcher);

	File f;

	auto bgImgPath = mpc::StartUp::resPath + "/img/bg.png";
	f = File(bgImgPath);
	bgImg = ImageFileFormat::loadFrom(f);

	dataWheel = new DataWheelControl(mpc::Mpc::instance().getHardware().lock()->getDataWheel(), "datawheel");
	mpc::Mpc::instance().getHardware().lock()->getDataWheel().lock()->addObserver(dataWheel);
	auto dataWheelImgPath = mpc::StartUp::resPath + "/img/datawheels.png";
	f = File(dataWheelImgPath);
	dataWheelImg = ImageFileFormat::loadFrom(f);
	dataWheel->setImage(dataWheelImg, 100);
	addAndMakeVisible(dataWheel);
	dataWheel->setInputCatcher(inputCatcher);

	auto sliderImgPath = mpc::StartUp::resPath + "/img/sliders.png";
	f = File(sliderImgPath);
	sliderImg = ImageFileFormat::loadFrom(f);
	slider = new SliderControl(mpc::Mpc::instance().getHardware().lock()->getSlider(), 0, "slider");
	slider->setImage(sliderImg);
	slider->setInputCatcher(inputCatcher);
	addAndMakeVisible(slider);

	auto recKnobImgPath = mpc::StartUp::resPath + "/img/recknobs.png";
	f = File(recKnobImgPath);
	recKnobImg = ImageFileFormat::loadFrom(f);
	recKnob = new KnobControl(0, mpc::Mpc::instance().getHardware().lock()->getRecPot(), mpc::Mpc::instance().getAudioMidiServices().lock()->getRecordLevel(), "recknob");
	recKnob->setImage(recKnobImg);
	addAndMakeVisible(recKnob);
	recKnob->setInputCatcher(inputCatcher);

	auto volKnobImgPath = mpc::StartUp::resPath + "/img/volknobs.png";
	f = File(volKnobImgPath);
	volKnobImg = ImageFileFormat::loadFrom(f);
	volKnob = new KnobControl(0, mpc::Mpc::instance().getHardware().lock()->getVolPot(), mpc::Mpc::instance().getAudioMidiServices().lock()->getMasterLevel(), "volknob");
	volKnob->setImage(volKnobImg);
	addAndMakeVisible(volKnob);
	volKnob->setInputCatcher(inputCatcher);

	auto padHitImgPath = mpc::StartUp::resPath + "/img/padhit.png";
	f = File(padHitImgPath);
	padHitImg = ImageFileFormat::loadFrom(f);

	auto ledRedImgPath = mpc::StartUp::resPath + "/img/led_red.png";
	f = File(ledRedImgPath);
	ledRedImg = ImageFileFormat::loadFrom(f);
	auto ledGreenImgPath = mpc::StartUp::resPath + "/img/led_green.png";
	f = File(ledGreenImgPath);
	ledGreenImg = ImageFileFormat::loadFrom(f);

	leds = new LedControl(ledGreenImg, ledRedImg, inputCatcher);
	leds->setPadBankA(true);
	leds->addAndMakeVisible(this);
	for (auto& l : mpc::Mpc::instance().getHardware().lock()->getLeds()) {
		l->addObserver(leds);
	}

	ButtonControl::initRects();
	std::vector<std::string> buttonLabels{ "left", "right", "up", "down", "rec", "overdub", "stop", "play", "playstart", "mainscreen", "prevstepevent", "nextstepevent",	"goto",	"prevbarstart",	"nextbarend", "tap", "nextseq",	"trackmute", "openwindow", "fulllevel", "sixteenlevels", "f1", "f2", "f3", "f4", "f5", "f6", "shift", "enter", "undoseq", "erase", "after", "banka", "bankb", "bankc", "bankd" };
	for (auto& l : buttonLabels) {
		auto bc = new ButtonControl(*ButtonControl::rects[l], mpc::Mpc::instance().getHardware().lock()->getButton(l), l);
		addAndMakeVisible(bc);
		bc->setInputCatcher(inputCatcher);
		buttons.push_back(bc);
	}

	const int padWidth = 96;
	int padSpacing = 25;
	const int padOffsetX = 778;
	const int padOffsetY = 397;
	int padCounter = 0;
	for (int j = 3; j >= 0; j--) {
		for (int i = 0; i < 4; i++) {
			int x1 = (padWidth + padSpacing) * i + padOffsetX + i;
			int y1 = (padWidth + padSpacing) * j + padOffsetY;
			Rectangle<float> rect(x1, y1, padWidth + i, padWidth);
			auto pc = new PadControl(rect, mpc::Mpc::instance().getHardware().lock()->getPad(padCounter++), padHitImg, "pad");
			addAndMakeVisible(pc);
			pads.push_back(pc);
		}
	}

	lcd = new LCDControl("lcd", mpc::Mpc::instance().getLayeredScreen());
	lcd->setSize(496, 120);
	lcd->setInputCatcher(inputCatcher);
	addAndMakeVisible(lcd);

	versionLabel.setText("v" + string(ProjectInfo::versionString), dontSendNotification);
	versionLabel.setColour(Label::textColourId, Colours::darkgrey);
	addAndMakeVisible(versionLabel);

    setResizable(true, true);

	setSize(p.lastUIWidth, p.lastUIHeight);
	setResizeLimits(1298 / 2, 994 / 2, 1298, 994);
	getConstrainer()->setFixedAspectRatio(1.305835010060362);

	lcd->dirtyRect = Rectangle<int>(0, 0, 248, 60);
	lcd->drawPixelsToImg();
	lcd->startTimer(50);
}

VmpcAudioProcessorEditor::~VmpcAudioProcessorEditor()
{
	mpcSplashScreen.deleteAndZero();
	lcd->stopTimer();
	delete dataWheel;
	delete lcd;
	for (auto& l : mpc::Mpc::instance().getHardware().lock()->getLeds()) {
		l->deleteObserver(leds);
	}
	delete leds;
	delete recKnob;
	delete volKnob;
	delete slider;
	for (auto& b : buttons) {
		delete b;
	}
	for (auto& p : pads) {
		delete p;
	}
	delete inputCatcher;
}

void VmpcAudioProcessorEditor::initialise()
{
	auto bgImgPath = mpc::StartUp::resPath + "/img/disclaimer.gif";
	auto disclaimer = ImageFileFormat::loadFrom(File(bgImgPath));
	mpcSplashScreen = new SplashScreen("Woah", disclaimer, true);
	mpcSplashScreen->deleteAfterDelay(RelativeTime::seconds(8), true);
}

void VmpcAudioProcessorEditor::paint (Graphics& g)
{
	if (!initialFocusSet) {
		inputCatcher->grabKeyboardFocus();
		initialFocusSet = true;
	}
	g.drawImageWithin(bgImg, 0, 0, getWidth(), getHeight(), RectanglePlacement(RectanglePlacement::centred));
}

void VmpcAudioProcessorEditor::resized()
{
	getProcessor().lastUIWidth = getWidth();
	getProcessor().lastUIHeight = getHeight();
	double ratio = getWidth() / 1298.0;
	auto scaleTransform = AffineTransform::scale(ratio);
	inputCatcher->setBounds(0, 0, getWidth(), getHeight()); // don't transform! or kb events are partiallly gone
	dataWheel->setTransform(scaleTransform);
	dataWheel->setBounds(Constants::DATAWHEEL_RECT()->getX(), Constants::DATAWHEEL_RECT()->getY(), dataWheel->getFrameWidth(), dataWheel->getFrameHeight());
	slider->setTransform(scaleTransform);
	slider->setBounds(Constants::SLIDER_RECT()->getX(), Constants::SLIDER_RECT()->getY(), sliderImg.getWidth(), sliderImg.getHeight() / 100);
	recKnob->setTransform(scaleTransform);
	recKnob->setBounds(Constants::RECKNOB_RECT()->getX(), Constants::RECKNOB_RECT()->getY(), recKnobImg.getWidth(), recKnobImg.getHeight() / 100);
	volKnob->setTransform(scaleTransform);
	volKnob->setBounds(Constants::VOLKNOB_RECT()->getX(), Constants::VOLKNOB_RECT()->getY(), volKnobImg.getWidth(), volKnobImg.getHeight() / 100);
	lcd->setTransform(scaleTransform);
	lcd->setBounds(Constants::LCD_RECT()->getX(), Constants::LCD_RECT()->getY(), 496, 120);
	for (auto& b : buttons) {
		b->setTransform(scaleTransform);
		b->setBounds();
	}
	for (auto& p : pads) {
		p->setTransform(scaleTransform);
		p->setBounds();
	}

	leds->setTransform(scaleTransform);
	leds->setBounds();

	versionLabel.setTransform(scaleTransform);
	versionLabel.setBounds(1175, 118, 100, 20);
}
