#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "gui/Constants.h"

#include <hardware/Hardware.hpp>
#include <hardware/Led.hpp>
#include <controls/Controls.hpp>
#include <audiomidi/AudioMidiServices.hpp>
#include <Paths.hpp>
#include "version.h"

using namespace std;

VmpcAudioProcessorEditor::VmpcAudioProcessorEditor(VmpcAudioProcessor& p)
    : AudioProcessorEditor(&p), vmpcAudioProcessor(p), mpc(p.mpc)
{
	setWantsKeyboardFocus(false);
	initialise();

	keyEventListener = new KeyEventListener(mpc.getControls().lock()->getKeyEventHandler());
	keyEventListener->setSize(1298, 994);
	keyEventListener->setWantsKeyboardFocus(true);
	addAndMakeVisible(keyEventListener);

	File f;

	auto bgImgPath = mpc::Paths::resPath() + "/img/bg.png";
	f = File(bgImgPath);
	bgImg = ImageFileFormat::loadFrom(f);

    auto keyboardImgPath = mpc::Paths::resPath() + "/img/keyboard.png";
    f = File(keyboardImgPath);
    keyboardImg = ImageFileFormat::loadFrom(f);
    
    keyboardButton.setImages(false, true, true, keyboardImg, 0.5, Colours::transparentWhite, keyboardImg, 1.0, Colours::transparentWhite, keyboardImg, 0.25, Colours::transparentWhite);
    
	dataWheel = new DataWheelControl(mpc.getHardware().lock()->getDataWheel(), "datawheel");
	mpc.getHardware().lock()->getDataWheel().lock()->addObserver(dataWheel);
	auto dataWheelImgPath = mpc::Paths::resPath() + "/img/datawheels.png";
	f = File(dataWheelImgPath);
	dataWheelImg = ImageFileFormat::loadFrom(f);
	dataWheel->setImage(dataWheelImg, 100);
	addAndMakeVisible(dataWheel);

	auto sliderImgPath = mpc::Paths::resPath() + "/img/sliders.png";
	f = File(sliderImgPath);
	sliderImg = ImageFileFormat::loadFrom(f);
	slider = new SliderControl(mpc.getHardware().lock()->getSlider(), 0, "slider");
	slider->setImage(sliderImg);
	addAndMakeVisible(slider);

	auto recKnobImgPath = mpc::Paths::resPath() + "/img/recknobs.png";
	f = File(recKnobImgPath);
	recKnobImg = ImageFileFormat::loadFrom(f);
	recKnob = new KnobControl(0, mpc.getHardware().lock()->getRecPot(), mpc.getAudioMidiServices().lock()->getRecordLevel(), "recknob");
	recKnob->setImage(recKnobImg);
	addAndMakeVisible(recKnob);

	auto volKnobImgPath = mpc::Paths::resPath() + "/img/volknobs.png";
	f = File(volKnobImgPath);
	volKnobImg = ImageFileFormat::loadFrom(f);
	volKnob = new KnobControl(0, mpc.getHardware().lock()->getVolPot(), mpc.getAudioMidiServices().lock()->getMasterLevel(), "volknob");
	volKnob->setImage(volKnobImg);
	addAndMakeVisible(volKnob);

	auto padHitImgPath = mpc::Paths::resPath() + "/img/padhit.png";
	f = File(padHitImgPath);
	padHitImg = ImageFileFormat::loadFrom(f);

	auto ledRedImgPath = mpc::Paths::resPath() + "/img/led_red.png";
	f = File(ledRedImgPath);
	ledRedImg = ImageFileFormat::loadFrom(f);
	auto ledGreenImgPath = mpc::Paths::resPath() + "/img/led_green.png";
	f = File(ledGreenImgPath);
	ledGreenImg = ImageFileFormat::loadFrom(f);

	leds = new LedControl(ledGreenImg, ledRedImg);
	leds->setPadBankA(true);
	leds->addAndMakeVisible(this);
    
	for (auto& l : mpc.getHardware().lock()->getLeds())
    {
		l->addObserver(leds);
    }

	ButtonControl::initRects();

    std::vector<std::string> buttonLabels{ "left", "right", "up", "down", "rec", "overdub", "stop", "play", "play-start", "main-screen", "prev-step-event", "next-step-event",	"go-to", "prev-bar-start", "next-bar-end", "tap", "next-seq", "track-mute", "open-window", "full-level", "sixteen-levels", "f1", "f2", "f3", "f4", "f5", "f6", "shift", "enter", "undo-seq", "erase", "after", "bank-a", "bank-b", "bank-c", "bank-d" };
	
    for (auto& l : buttonLabels)
    {
		auto bc = new ButtonControl(*ButtonControl::rects[l], mpc.getHardware().lock()->getButton(l), l);
		addAndMakeVisible(bc);
		buttons.push_back(bc);
	}

	const int padWidth = 96;
	int padSpacing = 25;
	const int padOffsetX = 778;
	const int padOffsetY = 397;
	int padCounter = 0;
    
	for (int j = 3; j >= 0; j--)
    {
		for (int i = 0; i < 4; i++)
        {
			int x1 = (padWidth + padSpacing) * i + padOffsetX + i;
			int y1 = (padWidth + padSpacing) * j + padOffsetY;
			Rectangle<float> rect(x1, y1, padWidth + i, padWidth);
		
            auto pc = new PadControl(mpc, rect, mpc.getHardware().lock()->getPad(padCounter++), padHitImg, "pad");
			addAndMakeVisible(pc);

            pads.push_back(pc);
		}
	}

	lcd = new LCDControl(mpc, "lcd", mpc.getLayeredScreen());
	lcd->setSize(496, 120);
	addAndMakeVisible(lcd);

	versionLabel.setText(version::get(), dontSendNotification);
	versionLabel.setColour(Label::textColourId, Colours::darkgrey);
	addAndMakeVisible(versionLabel);
    
    setResizable(true, true);

	setSize(p.lastUIWidth, p.lastUIHeight);
	setResizeLimits(1298 / 2, 994 / 2, 1298, 994);
	getConstrainer()->setFixedAspectRatio(1.305835010060362);
    
    keyboardButton.setBounds(p.lastUIWidth - 80, 10, 75, 30);
    
    class KbButtonListener : public Button::Listener {
    public:
        KbButtonListener(mpc::Mpc& _mpc) : mpc(_mpc) {}
        mpc::Mpc& mpc;
        void buttonClicked(Button*) override {
            mpc.getLayeredScreen().lock()->openScreen("vmpc-keyboard");
        }
    };
    
    keyboardButton.addListener(new KbButtonListener(mpc));
    keyboardButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(keyboardButton);
    
	lcd->drawPixelsToImg();
	lcd->startTimer(25);
	
	if (vmpcAudioProcessor.poweredUp)
	{
		lcd->skipPowerUpSequence();
	}
	else {
		lcd->startPowerUpSequence();
        vmpcAudioProcessor.poweredUp = true;
	}
}

VmpcAudioProcessorEditor::~VmpcAudioProcessorEditor()
{
	delete keyEventListener;
	mpcSplashScreen.deleteAndZero();
	lcd->stopTimer();

	delete dataWheel;
	delete lcd;

	for (auto& l : mpc.getHardware().lock()->getLeds())
	{
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
}

void VmpcAudioProcessorEditor::initialise()
{
	if (vmpcAudioProcessor.shouldShowDisclaimer)
	{
		auto disclaimerImgPath = mpc::Paths::resPath() + "/img/disclaimer.gif";
		auto disclaimerImg = ImageFileFormat::loadFrom(File(disclaimerImgPath));
		mpcSplashScreen = new SplashScreen("Disclaimer", disclaimerImg, true);
		mpcSplashScreen->setWantsKeyboardFocus(false);
		mpcSplashScreen->deleteAfterDelay(RelativeTime::seconds(8), true);
        vmpcAudioProcessor.shouldShowDisclaimer = false;
	}

	if (TopLevelWindow::getNumTopLevelWindows() != 0 && TopLevelWindow::getTopLevelWindow(0) != nullptr)
	{
		TopLevelWindow::getTopLevelWindow(0)->setWantsKeyboardFocus(false);
	}
}

void VmpcAudioProcessorEditor::paint (Graphics& g)
{
	g.drawImageWithin(bgImg, 0, 0, getWidth(), getHeight(), RectanglePlacement(RectanglePlacement::centred));
}

void VmpcAudioProcessorEditor::resized()
{
	getProcessor().lastUIWidth = getWidth();
	getProcessor().lastUIHeight = getHeight();
    auto ratio = static_cast<float>(getWidth() / 1298.0);
	auto scaleTransform = AffineTransform::scale(ratio);
	keyEventListener->setBounds(0, 0, getWidth(), getHeight()); // don't transform! or kb events are partiallly gone
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
    
    keyboardButton.setBounds(getWidth() - 80, 10, 75, 30);
    lcd->setTransform(scaleTransform);
    
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
