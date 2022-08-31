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

  showDisclaimer();

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

void VmpcAudioProcessorEditor::showDisclaimer()
{
  if (vmpcAudioProcessor.shouldShowDisclaimer)
  {
    class VmpcSplashScreen : public juce::Component, juce::Timer {
    public:
      juce::Image img;
      VmpcSplashScreen() {
        img = ResourceUtil::loadImage("img/disclaimer.gif");
        startTimer(8000);
      }
      
      void mouseDown(const juce::MouseEvent&) override {
        delete this;
      }
      
      void paint(juce::Graphics& g) override {
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        g.setOpacity (1.0f);
        g.drawImage (img, getLocalBounds().toFloat(), juce::RectanglePlacement (juce::RectanglePlacement::fillDestination));
      }
      
      void timerCallback() override {
        stopTimer();
        callAfterDelay(10, [this](){
          delete this;
        });
      }
    };
    
    vmpcSplashScreen = new VmpcSplashScreen();
    
    vmpcSplashScreen->setWantsKeyboardFocus(false);
    vmpcAudioProcessor.shouldShowDisclaimer = false;
    
    addAndMakeVisible(vmpcSplashScreen);
  }
  
  if (juce::TopLevelWindow::getNumTopLevelWindows() != 0 &&
      juce::TopLevelWindow::getTopLevelWindow(0) != nullptr)
    juce::TopLevelWindow::getTopLevelWindow(0)->setWantsKeyboardFocus(false);
}

void VmpcAudioProcessorEditor::resized()
{
  if (juce::SystemStats::getOperatingSystemType() == juce::SystemStats::OperatingSystemType::iOS)
  {
    auto primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
  
    if (primaryDisplay != nullptr)
    {
      auto area = primaryDisplay->userArea;
      setSize(area.getWidth(), area.getHeight());
      viewport.setBounds(primaryDisplay->userArea);
      
      auto portrait = area.getWidth() < area.getHeight();
      auto ratio = 1298.0 / 994.0;
      
      if (portrait)
      {
        viewport.getViewedComponent()->setBounds(0, 0, area.getHeight() * ratio, area.getHeight());
      }
      else
      {
        viewport.getViewedComponent()->setBounds(0, 0, area.getWidth(), getWidth() / ratio);
      }
    }
  }
  else
  {
    getProcessor().lastUIWidth = getWidth();
    getProcessor().lastUIHeight = getHeight();
    viewport.setBounds(0, 0, getWidth(), getHeight());
    viewport.getViewedComponent()->setBounds(0, 0, getWidth(), getHeight());
  }
  
  if (vmpcSplashScreen && vmpcSplashScreen->isVisible())
  {
    int width = 468;
    int height = 160;
    auto c = getBounds().getCentre();
    vmpcSplashScreen->setBounds(c.getX() - (width / 2), c.getY() - (height / 2), width, height);
  }
}
