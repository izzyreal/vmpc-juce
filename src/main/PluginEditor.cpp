#include "PluginEditor.h"

#include "PluginProcessor.h"

#include "gui/Constants.h"

#include <hardware/Hardware.hpp>
#include <audiomidi/AudioMidiServices.hpp>
#include <Paths.hpp>

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
  
  setWantsKeyboardFocus(false);
  
  initialise();
  
  //  setResizable(true, true);
  //  setResizeLimits(1298 / 2, 994 / 2, 1298, 994);
  //  getConstrainer()->setFixedAspectRatio(1.305835010060362);
  
  //
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
}

VmpcAudioProcessorEditor::~VmpcAudioProcessorEditor()
{
  mpcSplashScreen.deleteAndZero();
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
}
