#include "ContentComponent.hpp"
#include "Constants.hpp"
#include "../ResourceUtil.hpp"

#include <Mpc.hpp>

#include <controls/Controls.hpp>
#include <controls/KeyEvent.hpp>
#include <controls/KeyEventHandler.hpp>

#include <hardware/Hardware.hpp>
#include <hardware/Led.hpp>

#include "../version.h"

#include <raw_keyboard_input/raw_keyboard_input.h>

ContentComponent::ContentComponent(mpc::Mpc &_mpc, std::function<void()>& showAudioSettingsDialog)
        : mpc(_mpc), keyEventHandler(mpc.getControls()->getKeyEventHandler())
{
    setName("ContentComponent");

    keyboard = KeyboardFactory::instance(this);

    keyboard->onKeyDownFn = [&](int keyCode) {
        keyEventHandler.lock()->handle(mpc::controls::KeyEvent(keyCode, true));
    };

    keyboard->onKeyUpFn = [&](int keyCode) {
        keyEventHandler.lock()->handle(mpc::controls::KeyEvent(keyCode, false));
    };

    setWantsKeyboardFocus(true);

    background = new Background(mpc);
    addAndMakeVisible(background);

    dataWheel = new DataWheelControl(mpc, mpc.getHardware()->getDataWheel());

    const auto dataWheelSkinPath = mpc.paths->appDocumentsPath() / "Skin" / "datawheels.png";
    const bool dataWheelSkinExists = fs::exists(dataWheelSkinPath);

    if (dataWheelSkinExists)
    {
        const auto skinData = get_file_data(dataWheelSkinPath);
        dataWheelImg = juce::ImageFileFormat::loadFrom(&skinData[0], skinData.size());
    }
    else
    {
        dataWheelImg = vmpc::ResourceUtil::loadImage("img/datawheels.jpg");
    }

    dataWheel->setImage(dataWheelImg, 100);
    addAndMakeVisible(dataWheel);

    lcd = new LCDControl(mpc);
    lcd->setSize(496, 120);
    addAndMakeVisible(lcd);

    lcd->drawPixelsToImg();
    lcd->startTimer(25);

    ButtonControl::initRects();

    for (auto &l: mpc.getHardware()->getButtonLabels())
    {
        if (ButtonControl::rects.find(l) == ButtonControl::rects.end())
        {
            continue;
        }

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

    padHitImg = vmpc::ResourceUtil::loadImage("img/padhit.png");

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

    const auto sliderSkinPath = mpc.paths->appDocumentsPath() / "Skin" / "sliders.png";
    const bool sliderSkinExists = fs::exists(sliderSkinPath);

    if (sliderSkinExists)
    {
        const auto skinData = get_file_data(sliderSkinPath);
        sliderImg = juce::ImageFileFormat::loadFrom(&skinData[0], skinData.size());
    }
    else
    {
        sliderImg = vmpc::ResourceUtil::loadImage("img/sliders.jpg");
    }

    slider = new SliderControl(mpc.getHardware()->getSlider());
    slider->setImage(sliderImg);
    addAndMakeVisible(slider);

    recKnobImg = vmpc::ResourceUtil::loadImage("img/recknobs.jpg");
    recKnob = new KnobControl(mpc.getHardware()->getRecPot());
    recKnob->setImage(recKnobImg);
    addAndMakeVisible(recKnob);

    volKnobImg = vmpc::ResourceUtil::loadImage("img/volknobs.jpg");
    volKnob = new KnobControl(mpc.getHardware()->getVolPot());
    volKnob->setImage(volKnobImg);
    addAndMakeVisible(volKnob);

    ledRedImg = vmpc::ResourceUtil::loadImage("img/led_red.png");
    ledGreenImg = vmpc::ResourceUtil::loadImage("img/led_green.png");

    leds = new LedControl(mpc, ledGreenImg, ledRedImg);
    leds->setPadBankA(true);
    leds->addAndMakeVisible(this);

    for (auto &l: mpc.getHardware()->getLeds())
    {
      l->addObserver(leds);
    }

    leds->startTimer(25);
    slider->startTimer(25);

    versionLabel.setText(version::get(), juce::dontSendNotification);
    versionLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(versionLabel);
    
    topRightMenu = new TopRightMenu(mpc, showAudioSettingsDialog);
    
    addAndMakeVisible(topRightMenu);

    juce::Desktop::getInstance().addFocusChangeListener(this);
}

ContentComponent::~ContentComponent()
{
  for (auto &l: mpc.getHardware()->getLeds())
  {
    l->deleteObserver(leds);
  }
    
    delete topRightMenu;

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
    
    topRightMenu->setTransform(scaleTransform);
    topRightMenu->setBounds(0, 20, (getWidth() / scale) - 30, 70);

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
