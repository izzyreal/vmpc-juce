#include "VmpcEditor.h"

#include "ResourceUtil.h"

#include <hardware/Hardware.hpp>
#include <audiomidi/AudioMidiServices.hpp>

VmpcEditor::VmpcEditor(VmpcProcessor& p)
: AudioProcessorEditor(&p), vmpcProcessor(p), mpc(p.mpc)
{
  auto content = new ContentComponent(mpc, p.showAudioSettingsDialog);
  
  const bool deleteContentWhenNotUsedAnymore = true;
  viewport.setViewedComponent(content, deleteContentWhenNotUsedAnymore);
  viewport.setScrollBarsShown(false, false);
  viewport.setScrollOnDragEnabled(false);
  addAndMakeVisible(viewport);
  
  setWantsKeyboardFocus(false);

  showDisclaimer();

  setSize(p.lastUIWidth, p.lastUIHeight);
  setResizable(true, false);
  setResizeLimits(1298 / 2, 994 / 2, 1298, 994);
  getConstrainer()->setFixedAspectRatio(1.305835010060362);
}

VmpcEditor::~VmpcEditor()
{
  vmpcSplashScreen.deleteAndZero();
}

void VmpcEditor::showDisclaimer()
{
  if (vmpcProcessor.shouldShowDisclaimer)
  {
    class VmpcSplashScreen : public juce::Component, juce::Timer {
    public:
      juce::Image img;
      VmpcSplashScreen() {
        setWantsKeyboardFocus(false);
        img = ResourceUtil::loadImage("img/disclaimer.gif");
        startTimer(8000);
      }
      
      void mouseDown(const juce::MouseEvent&) override {
        setVisible(false);
      }
      
      void paint(juce::Graphics& g) override {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.setOpacity(1.0f);
        g.drawImageAt(img, 0, 0);
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
    vmpcProcessor.shouldShowDisclaimer = false;
    
    addAndMakeVisible(vmpcSplashScreen);
  }
  
  if (juce::TopLevelWindow::getNumTopLevelWindows() != 0 &&
      juce::TopLevelWindow::getTopLevelWindow(0) != nullptr)
    juce::TopLevelWindow::getTopLevelWindow(0)->setWantsKeyboardFocus(false);
}

void VmpcEditor::resized()
{
  if (juce::SystemStats::getOperatingSystemType() == juce::SystemStats::OperatingSystemType::iOS)
  {
    if (juce::PluginHostType::jucePlugInClientCurrentWrapperType == juce::AudioProcessor::wrapperType_AudioUnitv3)
    {
        auto ratio = 1298.0 / 994.0;
        viewport.getViewedComponent()->setBounds(0, 0, getWidth(), getWidth() / ratio);
        viewport.setBounds(0, 0, getWidth(), getWidth() / ratio);
        return;
    }
    
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
    auto new_w = getWidth();
    auto new_h = getWidth() / (1298/994.f);

    if (new_h > getHeight())
    {
      new_h = getHeight();
      new_w = getHeight() * (1298/994.f);
    }

    getProcessor().lastUIWidth = new_w;
    getProcessor().lastUIHeight = new_h;
    viewport.setBounds((getWidth() - new_w) / 2.f, (getHeight() - new_h) / 2.f, new_w, new_h);
    viewport.getViewedComponent()->setBounds(0, 0, new_w, new_h);
  }
  
  if (vmpcSplashScreen && vmpcSplashScreen->isVisible())
  {
    int width = 468;
    int height = 160;
    auto c = getBounds().getCentre();
    vmpcSplashScreen->setBounds(c.getX() - (width / 2), c.getY() - (height / 2), width, height);
  }
}
