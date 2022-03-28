#include "PluginEditor.h"

#include "PluginProcessor.h"

#include "ResourceUtil.h"

#include <hardware/Hardware.hpp>
#include <audiomidi/AudioMidiServices.hpp>
#include <Paths.hpp>

VmpcAudioProcessorEditor::VmpcAudioProcessorEditor(VmpcAudioProcessor& p)
: AudioProcessorEditor(&p), vmpcAudioProcessor(p), mpc(p.mpc)
{
  auto content = new ContentComponent(mpc);
  
  const bool deleteContentWhenNotUsedAnymore = true;
  viewport.setViewedComponent(content, deleteContentWhenNotUsedAnymore);
  viewport.setScrollBarsShown(false, false);
  viewport.setScrollOnDragEnabled(false);
  addAndMakeVisible(viewport);
  
  setWantsKeyboardFocus(false);
  
  initialise();
  
  if (juce::SystemStats::getOperatingSystemType() == juce::SystemStats::OperatingSystemType::iOS) {
    auto primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    if (primaryDisplay != nullptr) setBounds(primaryDisplay->userArea);
  } else {
    setResizable(true, true);
    setResizeLimits(1298 / 2, 994 / 2, 1298, 994);
    getConstrainer()->setFixedAspectRatio(1.305835010060362);
  }
}

VmpcAudioProcessorEditor::~VmpcAudioProcessorEditor()
{
  vmpcSplashScreen.deleteAndZero();
}

void VmpcAudioProcessorEditor::initialise()
{
  if (vmpcAudioProcessor.shouldShowDisclaimer)
  {
    auto disclaimerImg = ResourceUtil::loadImage("img/disclaimer.gif");
    
    auto userArea = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
    int width  = userArea.getWidth();
    
    if (width > disclaimerImg.getWidth() * 2)
      width = disclaimerImg.getWidth() * 2;
    
    auto height = width * ((float) disclaimerImg.getHeight() / disclaimerImg.getWidth());
    auto x = userArea.getCentreX() - (width / 2.f);
    auto y = userArea.getCentreY() - (height / 2.f);
    
    class VmpcSplashScreen : public juce::SplashScreen {
    public:
      juce::Image _img;
      VmpcSplashScreen(juce::String title, juce::Image& img, int x, int y, int w, int h): juce::SplashScreen(title, x, y, true), _img(img) {
        setBounds(x, y, w, h);
      }
      void paint(juce::Graphics& g) override {
        if (getWidth() >= _img.getWidth() * 2) g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        g.setOpacity (1.0f);
        g.drawImage (_img, getLocalBounds().toFloat(), juce::RectanglePlacement (juce::RectanglePlacement::fillDestination));
      }
    };
    
    vmpcSplashScreen = new VmpcSplashScreen("Disclaimer", disclaimerImg, x, y, width, height);
    
    vmpcSplashScreen->setWantsKeyboardFocus(false);
    vmpcSplashScreen->deleteAfterDelay(juce::RelativeTime::seconds(8), true);
    vmpcAudioProcessor.shouldShowDisclaimer = false;
  }
  
  if (juce::TopLevelWindow::getNumTopLevelWindows() != 0 &&
      juce::TopLevelWindow::getTopLevelWindow(0) != nullptr)
    juce::TopLevelWindow::getTopLevelWindow(0)->setWantsKeyboardFocus(false);
}

void VmpcAudioProcessorEditor::resized()
{
  if (juce::SystemStats::getOperatingSystemType() == juce::SystemStats::OperatingSystemType::iOS) {
    auto primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
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
  } else {
    getProcessor().lastUIWidth = getWidth();
    getProcessor().lastUIHeight = getHeight();
    viewport.getViewedComponent()->setBounds(0, 0, getWidth(), getHeight());
  }
}
