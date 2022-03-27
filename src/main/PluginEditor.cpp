#include "PluginEditor.h"

#include "PluginProcessor.h"

#include "gui/Constants.h"

#include <hardware/Hardware.hpp>
#include <hardware/Led.hpp>
#include <audiomidi/AudioMidiServices.hpp>
#include <Paths.hpp>
#include "version.h"

#include <cmrc/cmrc.hpp>
#include <string_view>

CMRC_DECLARE(vmpcjuce);

using namespace std;

Image loadImage(string path)
{
  auto fs = cmrc::vmpcjuce::get_filesystem();
  auto file = fs.open(path.c_str());
  auto data = string_view(file.begin(), file.size()).data();
  auto stream = MemoryInputStream(data, file.size(), true);
  return ImageFileFormat::loadFrom(stream);
}

VmpcAudioProcessorEditor::VmpcAudioProcessorEditor(VmpcAudioProcessor& p)
: AudioProcessorEditor(&p), vmpcAudioProcessor(p), mpc(p.mpc)
{
  
  vmpcAudioProcessor.shouldShowDisclaimer = false;
  
  struct Background : public Component {
    Image img = loadImage("img/bg.jpg");
    void paint(Graphics& g) override {
      g.drawImageWithin(img, 0, 0, getParentWidth(), getParentHeight(), RectanglePlacement::centred);
    }
  };
  
  struct ContentComponent : public Component {
    Background background;
    mpc::Mpc& mpc;
    KeyEventListener* keyEventListener = nullptr;
    ContentComponent(mpc::Mpc& _mpc) : mpc(_mpc) {
      addAndMakeVisible(background);
      
      keyEventListener = new KeyEventListener(mpc);
      keyEventListener->setWantsKeyboardFocus(true);
      addAndMakeVisible(keyEventListener);
    }
    ~ContentComponent() override {
      delete keyEventListener;
    }
    void resized() override {
      background.setSize(getWidth(), getHeight());
      keyEventListener->setSize(getWidth(), getHeight());
    }
  };
  
  auto content = new ContentComponent(mpc);
  
  const bool deleteContentWhenNotUsedAnymore = true;
  viewport.setViewedComponent(content, deleteContentWhenNotUsedAnymore);
  viewport.setScrollBarsShown(false, false);
  viewport.setScrollOnDragEnabled(false);
  addAndMakeVisible(viewport);
  
  auto primaryDisplay = Desktop::getInstance().getDisplays().getPrimaryDisplay();

  if (primaryDisplay != nullptr) {
    auto area = primaryDisplay->userArea;
    setSize(area.getWidth(), area.getHeight());
    
    auto portrait = area.getWidth() < area.getHeight();
    auto ratio = 1298.0 / 994.0;

    if (portrait) {
      content->setBounds(0, 0, area.getHeight() * ratio, area.getHeight());
    } else {
      content->setBounds(0, 0, area.getWidth(), getWidth() / ratio);
    }
  }

  
  
  
  
  /////////////////////////////////
  
  
  
  
  setWantsKeyboardFocus(false);
  
  initialise();
  
  bgImg = loadImage("img/bg.jpg");
  keyboardImg = loadImage("img/keyboard.png");
  
  keyboardButton.setImages(false, true, true, keyboardImg, 0.5, Colours::transparentWhite, keyboardImg, 1.0, Colours::transparentWhite, keyboardImg, 0.25, Colours::transparentWhite);
  
  resetWindowSizeImg = loadImage("img/reset-window-size.png");
  
  resetWindowSizeButton.setImages(false, true, true, resetWindowSizeImg, 0.5, Colours::transparentWhite, resetWindowSizeImg, 1.0, Colours::transparentWhite, resetWindowSizeImg, 0.25, Colours::transparentWhite);
  
  dataWheel = new DataWheelControl(mpc.getHardware().lock()->getDataWheel());
  mpc.getHardware().lock()->getDataWheel().lock()->addObserver(dataWheel);
  dataWheelImg = loadImage("img/datawheels.jpg");
  dataWheel->setImage(dataWheelImg, 100);
//  addAndMakeVisible(dataWheel);
  
  sliderImg = loadImage("img/sliders.jpg");
  slider = new SliderControl(mpc.getHardware().lock()->getSlider());
  slider->setImage(sliderImg);
//  addAndMakeVisible(slider);
  
  recKnobImg = loadImage("img/recknobs.jpg");
  recKnob = new KnobControl(0, mpc.getHardware().lock()->getRecPot());
  recKnob->setImage(recKnobImg);
//  addAndMakeVisible(recKnob);
  
  volKnobImg = loadImage("img/volknobs.jpg");
  volKnob = new KnobControl(0, mpc.getHardware().lock()->getVolPot());
  volKnob->setImage(volKnobImg);
//  addAndMakeVisible(volKnob);
  
  padHitImg = loadImage("img/padhit.png");
  
  ledRedImg = loadImage("img/led_red.png");
  ledGreenImg = loadImage("img/led_green.png");
  
  leds = new LedControl(mpc, ledGreenImg, ledRedImg);
  leds->setPadBankA(true);
//  leds->addAndMakeVisible(this);
  
  for (auto& l : mpc.getHardware().lock()->getLeds())
    l->addObserver(leds);
  
  ButtonControl::initRects();
  
  for (auto& l : mpc.getHardware().lock()->getButtonLabels())
  {
    auto bc = new ButtonControl(ButtonControl::rects[l]->expanded(10),
                                mpc.getHardware().lock()->getButton(l));
//    addAndMakeVisible(bc);
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
      
      auto pc = new PadControl(mpc, rect, mpc.getHardware().lock()->getPad(padCounter++), padHitImg);
//      addAndMakeVisible(pc);
      
      pads.push_back(pc);
    }
  }
  
  lcd = new LCDControl(mpc, mpc.getLayeredScreen());
  lcd->setSize(496, 120);
//  addAndMakeVisible(lcd);
  
  versionLabel.setText(version::get(), dontSendNotification);
  versionLabel.setColour(Label::textColourId, Colours::darkgrey);
//  addAndMakeVisible(versionLabel);
  
  //
  keyboardButton.setTooltip("Configure computer keyboard");
  
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
//  addAndMakeVisible(keyboardButton);
  
  //
  resetWindowSizeButton.setTooltip("Reset window size");
  
  class ResetButtonListener : public Button::Listener {
  public:
    ResetButtonListener(mpc::Mpc& _mpc, AudioProcessorEditor* __this) : mpc(_mpc), _this(__this) {}
    mpc::Mpc& mpc;
    AudioProcessorEditor* _this;
    void buttonClicked(Button*) override {
      _this->setSize(1298 / 2, 994 /2);
    }
  };
  
  resetWindowSizeButton.addListener(new ResetButtonListener(mpc, this));
  resetWindowSizeButton.setWantsKeyboardFocus(false);
//  addAndMakeVisible(resetWindowSizeButton);
  
//  setResizable(true, true);
//  setResizeLimits(1298 / 2, 994 / 2, 1298, 994);
//  getConstrainer()->setFixedAspectRatio(1.305835010060362);
  
  //
  lcd->drawPixelsToImg();
  lcd->startTimer(25);
  
  leds->startTimer(25);
  slider->startTimer(25);
}

VmpcAudioProcessorEditor::~VmpcAudioProcessorEditor()
{
  mpcSplashScreen.deleteAndZero();
  lcd->stopTimer();
  
  delete dataWheel;
  delete lcd;
  
  for (auto& l : mpc.getHardware().lock()->getLeds())
    l->deleteObserver(leds);
  
  delete leds;
  delete recKnob;
  delete volKnob;
  delete slider;
  
  for (auto& b : buttons)
    delete b;
  
  for (auto& p : pads)
    delete p;
}

void VmpcAudioProcessorEditor::initialise()
{
  if (vmpcAudioProcessor.shouldShowDisclaimer)
  {
    auto disclaimerImg = loadImage("img/disclaimer.gif");
    mpcSplashScreen = new SplashScreen("Disclaimer", disclaimerImg, true);
    mpcSplashScreen->setWantsKeyboardFocus(false);
    mpcSplashScreen->deleteAfterDelay(RelativeTime::seconds(8), true);
    vmpcAudioProcessor.shouldShowDisclaimer = false;
  }
  
  if (TopLevelWindow::getNumTopLevelWindows() != 0 &&
      TopLevelWindow::getTopLevelWindow(0) != nullptr)
    TopLevelWindow::getTopLevelWindow(0)->setWantsKeyboardFocus(false);
}

void VmpcAudioProcessorEditor::paint (Graphics& g)
{
//  g.drawImageWithin(bgImg, 0, 0, getWidth(), getHeight(), RectanglePlacement(RectanglePlacement::centred));
}

void VmpcAudioProcessorEditor::resized()
{
  auto primaryDisplay = Desktop::getInstance().getDisplays().getPrimaryDisplay();

  if (primaryDisplay != nullptr) {
    auto area = primaryDisplay->userArea;
    setSize(area.getWidth(), area.getHeight());
    viewport.setBounds(primaryDisplay->userArea);
    
    auto portrait = area.getWidth() < area.getHeight();
    auto ratio = 1298.0 / 994.0;

    if (portrait) {
      viewport.getViewedComponent()->setBounds(0, 0, area.getHeight() * ratio, area.getHeight());
    } else {
      viewport.getViewedComponent()->setBounds(0, 0, area.getWidth(), getWidth() / ratio);
    }
  }
  return;
  getProcessor().lastUIWidth = getWidth();
  getProcessor().lastUIHeight = getHeight();
  auto ratio = static_cast<float>(getWidth() / 1298.0);
  auto scaleTransform = AffineTransform::scale(ratio);
  dataWheel->setTransform(scaleTransform);
  dataWheel->setBounds(Constants::DATAWHEEL_RECT()->getX(), Constants::DATAWHEEL_RECT()->getY(), dataWheel->getFrameWidth(), dataWheel->getFrameHeight());
  
  slider->setTransform(scaleTransform);
  slider->setBounds(Constants::SLIDER_RECT()->getX(), Constants::SLIDER_RECT()->getY(), sliderImg.getWidth() / 2, sliderImg.getHeight() * 0.01 * 0.5);
  
  recKnob->setTransform(scaleTransform);
  recKnob->setBounds(Constants::RECKNOB_RECT()->getX(), Constants::RECKNOB_RECT()->getY(), recKnobImg.getWidth() / 2, recKnobImg.getHeight() * 0.01 * 0.5);
  
  volKnob->setTransform(scaleTransform);
  volKnob->setBounds(Constants::VOLKNOB_RECT()->getX(), Constants::VOLKNOB_RECT()->getY(), volKnobImg.getWidth() / 2, volKnobImg.getHeight() * 0.01 * 0.5);
  
  lcd->setTransform(scaleTransform);
  lcd->setBounds(Constants::LCD_RECT()->getX(), Constants::LCD_RECT()->getY(), 496, 120);
  
  keyboardButton.setBounds(1298 - (100 +  10), 10, 100, 50);
  keyboardButton.setTransform(scaleTransform);
  
  resetWindowSizeButton.setBounds(1298 - (145 + 20), 13, 45, 45);
  resetWindowSizeButton.setTransform(scaleTransform);
  
  lcd->setTransform(scaleTransform);
  
  for (auto& b : buttons)
  {
    b->setTransform(scaleTransform);
    b->setBounds();
  }
  
  for (auto& p : pads)
  {
    p->setBounds();
    p->setTransform(scaleTransform);
  }
  
  leds->setTransform(scaleTransform);
  leds->setBounds();
  
  versionLabel.setTransform(scaleTransform);
  versionLabel.setBounds(1175, 118, 100, 20);
}
