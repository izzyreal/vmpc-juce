#include "ContentComponent.h"
#include "Constants.h"
#include "../ResourceUtil.h"

#include <Mpc.hpp>

#include <disk/AbstractDisk.hpp>
#include <disk/MpcFile.hpp>

#include <controls/Controls.hpp>
#include <controls/KeyEvent.hpp>
#include <controls/KeyEventHandler.hpp>

#include <hardware/Hardware.hpp>
#include <hardware/Led.hpp>

#include <cmrc/cmrc.hpp>

#include "../version.h"

#include <raw_keyboard_input/raw_keyboard_input.h>

CMRC_DECLARE(vmpcjuce);

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define ENABLE_IMPORT 1
#endif
#endif

#if ENABLE_IMPORT
bool VmpcURLProcessor::destinationExists(const char* filename, const char* relativePath)
{
  auto newFilePath = fs::path(destinationDir).append(relativePath).append(filename);
  return fs::exists(newFilePath);
}

std::shared_ptr<std::ostream> VmpcURLProcessor::openOutputStream(const char* filename, const char* relativePath)
{
  auto newFileDir = fs::path(destinationDir).append(relativePath);
  fs::create_directories(newFileDir);
  auto newFilePath = newFileDir.append(filename);
  mpc::disk::MpcFile newFile(newFilePath);
  return newFile.getOutputStream();
}

void VmpcURLProcessor::initFiles()
{
    auto layeredScreen = mpc->getLayeredScreen();
    auto currentScreen = layeredScreen->getCurrentScreenName();
    if (currentScreen == "load" || currentScreen == "save" || currentScreen == "directory")
    {
        layeredScreen->openScreen(currentScreen == "directory" ? "load" : "black");
        mpc->getDisk()->initFiles();
        layeredScreen->openScreen(currentScreen);
    }
}
#endif

ContentComponent::ContentComponent(mpc::Mpc &_mpc, std::function<void()>& showAudioSettingsDialog)
        : mpc(_mpc), keyEventHandler(mpc.getControls()->getKeyEventHandler())
{
#if ENABLE_IMPORT
    urlProcessor.mpc = &mpc;
#endif
    setName("ContentComponent");

    keyboard = KeyboardFactory::instance(this);

    keyboard->onKeyDownFn = [&](int keyCode) {
        keyEventHandler.lock()->handle(mpc::controls::KeyEvent(keyCode, true));
    };

    keyboard->onKeyUpFn = [&](int keyCode) {
        keyEventHandler.lock()->handle(mpc::controls::KeyEvent(keyCode, false));
    };

    setWantsKeyboardFocus(true);

    background = new Background();
    addAndMakeVisible(background);

    dataWheel = new DataWheelControl(mpc, mpc.getHardware()->getDataWheel());
    dataWheelImg = ResourceUtil::loadImage("img/datawheels.jpg");
    dataWheel->setImage(dataWheelImg, 100);
    addAndMakeVisible(dataWheel);

    lcd = new LCDControl(mpc, keyboard);
    lcd->setSize(496, 120);
    addAndMakeVisible(lcd);

    lcd->drawPixelsToImg();
    lcd->startTimer(25);

    ButtonControl::initRects();

    for (auto &l: mpc.getHardware()->getButtonLabels())
    {
        auto bc = new ButtonControl(mpc, ButtonControl::rects[l]->expanded(10),
                                    mpc.getHardware()->getButton(l));
        addAndMakeVisible(bc);
        buttons.push_back(bc);
    }

    const int padWidth = 96;
    int padSpacing = 25;
    const int padOffsetX = 778;
    const int padOffsetY = 397;
    int padCounter = 0;

    padHitImg = ResourceUtil::loadImage("img/padhit.png");

    for (int j = 3; j >= 0; j--)
    {
        for (int i = 0; i < 4; i++)
        {
            int x1 = (padWidth + padSpacing) * i + padOffsetX + i;
            int y1 = (padWidth + padSpacing) * j + padOffsetY;
            juce::Rectangle<int> rect(x1, y1, padWidth + i, padWidth);

            auto pc = new PadControl(mpc, rect, mpc.getHardware()->getPad(padCounter++), padHitImg);
            addAndMakeVisible(pc);

            pads.push_back(pc);
        }
    }

    sliderImg = ResourceUtil::loadImage("img/sliders.jpg");
    slider = new SliderControl(mpc.getHardware()->getSlider());
    slider->setImage(sliderImg);
    addAndMakeVisible(slider);

    recKnobImg = ResourceUtil::loadImage("img/recknobs.jpg");
    recKnob = new KnobControl(mpc.getHardware()->getRecPot());
    recKnob->setImage(recKnobImg);
    addAndMakeVisible(recKnob);

    volKnobImg = ResourceUtil::loadImage("img/volknobs.jpg");
    volKnob = new KnobControl(mpc.getHardware()->getVolPot());
    volKnob->setImage(volKnobImg);
    addAndMakeVisible(volKnob);

    ledRedImg = ResourceUtil::loadImage("img/led_red.png");
    ledGreenImg = ResourceUtil::loadImage("img/led_green.png");

    leds = new LedControl(mpc, ledGreenImg, ledRedImg);
    leds->setPadBankA(true);
    leds->addAndMakeVisible(this);

    for (auto &l: mpc.getHardware()->getLeds())
    {
      l->addObserver(leds);
    }

    leds->startTimer(25);
    slider->startTimer(25);

    auto transparentWhite = juce::Colours::transparentWhite;

    keyboardImg = ResourceUtil::loadImage("img/keyboard.png");

    keyboardButton.setImages(false, true, true, keyboardImg, 0.5, transparentWhite, keyboardImg, 1.0, transparentWhite,
                             keyboardImg, 0.25, transparentWhite);

    resetWindowSizeImg = ResourceUtil::loadImage("img/reset-window-size.png");

    resetWindowSizeButton.setImages(false, true, true, resetWindowSizeImg, 0.5, transparentWhite, resetWindowSizeImg,
                                    1.0, transparentWhite, resetWindowSizeImg, 0.25, transparentWhite);
#if ENABLE_IMPORT
    importImg = ResourceUtil::loadImage("img/import.png");

    importButton.setImages(false, true, true, importImg, 0.5, transparentWhite, importImg, 1.0, transparentWhite,
                           importImg, 0.25, transparentWhite);

    importButton.setTooltip("Import files or folders");

    importButton.onClick = [&]() {
        auto uiView = getPeer()->getNativeHandle();
        doOpenIosDocumentBrowser(&urlProcessor, uiView);
    };

    addAndMakeVisible(importButton);
#endif
  
    versionLabel.setText(version::get(), juce::dontSendNotification);
    versionLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(versionLabel);

    if (juce::JUCEApplicationBase::isStandaloneApp())
    {
        gearImg = ResourceUtil::loadImage("img/gear.png");
        gearButton.setImages(false, true, true, gearImg, 0.5, transparentWhite, gearImg, 1.0, transparentWhite,
                             gearImg, 0.25, transparentWhite);
        gearButton.setTooltip("Audio/MIDI Settings");
        gearButton.onClick = [&showAudioSettingsDialog]() {
            showAudioSettingsDialog();
        };
        gearButton.setWantsKeyboardFocus(false);
        addAndMakeVisible(gearButton);
    }

    keyboardButton.setTooltip("Configure computer keyboard");
    keyboardButton.onClick = [&]() {
        mpc.getLayeredScreen()->openScreen("vmpc-keyboard");
    };

    keyboardButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(keyboardButton);

    if (juce::SystemStats::getOperatingSystemType() != juce::SystemStats::OperatingSystemType::iOS)
    {
        resetWindowSizeButton.setTooltip("Reset window size");
        resetWindowSizeButton.onClick = [&]() {
            getParentComponent()->getParentComponent()->getParentComponent()->setSize(1298 / 2, 994 / 2);
        };
        resetWindowSizeButton.setWantsKeyboardFocus(false);
        addAndMakeVisible(resetWindowSizeButton);
    }

    juce::Desktop::getInstance().addFocusChangeListener(this);
}

ContentComponent::~ContentComponent()
{
  for (auto &l: mpc.getHardware()->getLeds())
  {
    l->deleteObserver(leds);
  }

  juce::Desktop::getInstance().removeFocusChangeListener(this);
    delete keyboard;
    delete dataWheel;

    lcd->stopTimer();
    delete lcd;

    for (auto &b: buttons)
        delete b;

    for (auto &p: pads)
        delete p;

    delete leds;
    delete recKnob;
    delete volKnob;
    delete slider;
    delete background;
}

bool ContentComponent::keyPressed(const juce::KeyPress &k)
{
#if JUCE_IOS
    return false;
#else
    auto desc = k.getTextDescription().toStdString();

    if (desc == "command + Q" || desc == "alt + F4")
        return false;

    return true;
#endif
}

void ContentComponent::resized()
{
    auto scale = static_cast<float>(getWidth() / 1298.0);
    auto scaleTransform = juce::AffineTransform::scale(scale);
    background->setSize(getWidth(), getHeight());
    dataWheel->setTransform(scaleTransform);
    dataWheel->setBounds(Constants::dataWheelRect().getX(), Constants::dataWheelRect().getY(),
                         dataWheel->getFrameWidth(), dataWheel->getFrameHeight());
    lcd->setTransform(scaleTransform);
    lcd->setBounds(Constants::lcdRect().getX(), Constants::lcdRect().getY(), 496, 120);

    for (auto &b: buttons)
    {
        b->setTransform(scaleTransform);
        b->setBounds();
    }

    for (auto &p: pads)
    {
        p->setBounds();
        p->setTransform(scaleTransform);
    }

    leds->setTransform(scaleTransform);
    leds->setBounds();

    slider->setTransform(scaleTransform);
    slider->setBounds(Constants::sliderRect().getX(), Constants::sliderRect().getY(), sliderImg.getWidth() / 2,
                      sliderImg.getHeight() * 0.01 * 0.5);

    recKnob->setTransform(scaleTransform);
    recKnob->setBounds(Constants::recKnobRect().getX(), Constants::recKnobRect().getY(), recKnobImg.getWidth() / 2,
                       recKnobImg.getHeight() * 0.01 * 0.5);

    volKnob->setTransform(scaleTransform);
    volKnob->setBounds(Constants::volKnobRect().getX(), Constants::volKnobRect().getY(), volKnobImg.getWidth() / 2,
                       volKnobImg.getHeight() * 0.01 * 0.5);

    keyboardButton.setBounds(1298 - (100 + 10), 10, 100, 50);
    keyboardButton.setTransform(scaleTransform);

    if (juce::SystemStats::getOperatingSystemType() != juce::SystemStats::OperatingSystemType::iOS)
    {
        resetWindowSizeButton.setBounds(1298 - (145 + 20), 13, 45, 45);
        resetWindowSizeButton.setTransform(scaleTransform);
    }

    if (juce::JUCEApplicationBase::isStandaloneApp())
    {
        gearButton.setBounds(1298 - (190 + 30), 13, 45, 45);
        gearButton.setTransform(scaleTransform);
    }

#if ENABLE_IMPORT
    importButton.setBounds(1298 - (145 + 20), 10, 50, 50);
    importButton.setTransform(scaleTransform);
#endif

    versionLabel.setTransform(scaleTransform);
    versionLabel.setBounds(1152, 114, 100, 20);
}

void ContentComponent::globalFocusChanged(juce::Component *newFocus)
{
    if (newFocus != nullptr)
    {
#if JUCE_IOS
        // For some reason on iOS the ComboBoxes in the audio/midi settings panel grab focus
        // and on other systems they don't.
        if (newFocus->getName().isEmpty() && dynamic_cast<juce::ComboBox*>(newFocus))
        {
            return;
        }
#endif
        
        if (newFocus->getName() != "ContentComponent" &&
            newFocus->getName() != "auxlcdwindow" &&
            newFocus->getComponentID() != "AudioMidiSettingsWindow")
        {
            grabKeyboardFocus();
        }
    }

    if (keyboard == nullptr)
    {
        keyboard = KeyboardFactory::instance(this);

        keyboard->onKeyDownFn = [&](int keyCode) {
            keyEventHandler.lock()->handle(mpc::controls::KeyEvent(keyCode, true));
        };

        keyboard->onKeyUpFn = [&](int keyCode) {
            keyEventHandler.lock()->handle(mpc::controls::KeyEvent(keyCode, false));
        };
    }

    keyboard->allKeysUp();
}
