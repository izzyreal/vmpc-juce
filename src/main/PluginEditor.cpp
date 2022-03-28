#include "PluginEditor.h"

#include "PluginProcessor.h"

#include "gui/Constants.h"

#include <hardware/Hardware.hpp>
#include <audiomidi/AudioMidiServices.hpp>
#include <Paths.hpp>

VmpcAudioProcessorEditor::VmpcAudioProcessorEditor(VmpcAudioProcessor& p)
: AudioProcessorEditor(&p), vmpcAudioProcessor(p), mpc(p.mpc)
{
  
  vmpcAudioProcessor.shouldShowDisclaimer = false;

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
  auto primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
  
  if (primaryDisplay != nullptr)
    setBounds(primaryDisplay->userArea);
}

VmpcAudioProcessorEditor::~VmpcAudioProcessorEditor()
{
  mpcSplashScreen.deleteAndZero();
}

void VmpcAudioProcessorEditor::initialise()
{
  if (vmpcAudioProcessor.shouldShowDisclaimer)
  {
//    auto disclaimerImg = loadImage("img/disclaimer.gif");
//    mpcSplashScreen = new juce::SplashScreen("Disclaimer", disclaimerImg, true);
//    mpcSplashScreen->setWantsKeyboardFocus(false);
//    mpcSplashScreen->deleteAfterDelay(juce::RelativeTime::seconds(8), true);
//    vmpcAudioProcessor.shouldShowDisclaimer = false;
  }
  
  if (juce::TopLevelWindow::getNumTopLevelWindows() != 0 &&
      juce::TopLevelWindow::getTopLevelWindow(0) != nullptr)
    juce::TopLevelWindow::getTopLevelWindow(0)->setWantsKeyboardFocus(false);
}

void VmpcAudioProcessorEditor::resized()
{
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
}
