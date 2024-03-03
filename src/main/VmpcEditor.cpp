#include "VmpcEditor.h"

#include "ResourceUtil.h"

#include <hardware/Hardware.hpp>
#include <audiomidi/AudioMidiServices.hpp>

VmpcEditor::VmpcEditor(VmpcProcessor& p)
: AudioProcessorEditor(&p), vmpcProcessor(p), mpc(p.mpc)
{
  HMODULE hModule = LoadLibrary(TEXT("hello_resources.dll"));
  HRSRC hRes = FindResource(hModule, TEXT("resources/img/disclaimer.gif"), RT_RCDATA);
  HGLOBAL hData = LoadResource(hModule, hRes);
  void* pData = LockResource(hData);
  DWORD dataSize = SizeofResource(hModule, hRes);
  FreeLibrary(hModule);

  auto content = new ContentComponent(mpc, p.showAudioSettingsDialog);
  
  const bool deleteContentWhenNotUsedAnymore = true;
  viewport.setViewedComponent(content, deleteContentWhenNotUsedAnymore);
  viewport.setScrollBarsShown(false, false);
  viewport.setScrollOnDragEnabled(false);
  addAndMakeVisible(viewport);
  
  setWantsKeyboardFocus(false);

  showDisclaimer();

  setSize(p.lastUIWidth, p.lastUIHeight);
  setResizable(true, true);
  setResizeLimits(1298 / 2, 994 / 2, 1298, 994);
  getConstrainer()->setFixedAspectRatio(1.305835010060362);
  setLookAndFeel(&lookAndFeel);
}

VmpcEditor::~VmpcEditor()
{
  setLookAndFeel(nullptr);
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
        img = vmpc::ResourceUtil::loadImage("img/disclaimer.gif");
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
    const auto ratio = 1298.0 / 994.0;

#if JUCE_IOS
    if (juce::PluginHostType::jucePlugInClientCurrentWrapperType == juce::AudioProcessor::wrapperType_AudioUnitv3)
    {
        auto new_w = getWidth();
        auto new_h = new_w / ratio;
        
        if (new_h > getHeight())
        {
            new_h = getHeight();
            new_w = new_h * ratio;
        }
        
        viewport.setBounds((getWidth() - new_w) / 2, (getHeight() - new_h) / 2, getWidth(), getHeight());
        viewport.getViewedComponent()->setBounds(0, 0, new_w, new_h);
        return;
    }
    
    const auto primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
  
    if (primaryDisplay != nullptr)
    {
      const auto area = primaryDisplay->userArea;
      setSize(area.getWidth(), area.getHeight());
      viewport.setBounds(primaryDisplay->userArea);
      
      viewport.getViewedComponent()->setBounds(0, 0, area.getWidth(), getWidth() / ratio);
    }
#else
    auto new_w = getWidth();
    auto new_h = getWidth() / ratio;

    if (new_h > getHeight())
    {
      new_h = getHeight();
      new_w = getHeight() * ratio;
    }

    getProcessor().lastUIWidth = new_w;
    getProcessor().lastUIHeight = new_h;
    viewport.setBounds((getWidth() - new_w) / 2.f, (getHeight() - new_h) / 2.f, new_w, new_h);
    viewport.getViewedComponent()->setBounds(0, 0, new_w, new_h);
#endif
    
  if (vmpcSplashScreen != nullptr && vmpcSplashScreen->isVisible())
  {
    int width = 468;
    int height = 160;
    auto c = getBounds().getCentre();
    vmpcSplashScreen->setBounds(c.getX() - (width / 2), c.getY() - (height / 2), width, height);
  }
}
