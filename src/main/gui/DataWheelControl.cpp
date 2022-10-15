#include "DataWheelControl.h"

#include <Logger.hpp>

DataWheelControl::DataWheelControl(mpc::Mpc& mpc, std::weak_ptr<mpc::hardware::DataWheel> _dataWheel)
: VmpcTooltipComponent(mpc, std::make_shared<DummyDataWheelHwComponent>(mpc)), numFrames(0), frameWidth(0), frameHeight(0), dataWheel (_dataWheel)
{
    dataWheel.lock()->updateUi = [this](int increment) {
        juce::MessageManager::callAsync ([this, increment] {
            DataWheelControl* dataWheelControl = dynamic_cast<DataWheelControl*>(this);
            if (dataWheelControl != nullptr)
            {
                dataWheelControl->updateUI(increment);
            }
        });
    };
}

void DataWheelControl::mouseDown(const juce::MouseEvent& event)
{
  mouseDownEventSources.emplace(event.source.getIndex());
  if (latestMouseDownTime == juce::Time(0)) latestMouseDownTime = event.mouseDownTime;
}

void DataWheelControl::mouseUp(const juce::MouseEvent& event)
{
  mouseDownEventSources.erase(event.source.getIndex());
    
  if (mouseDownEventSources.empty())
  {
    latestMouseDownTime = juce::Time(0);
  }
  
  lastDy = 0;
  getParentComponent()->mouseUp(event);
}

void DataWheelControl::mouseDrag(const juce::MouseEvent& event)
{
  if (mouseDownEventSources.size() > 1 && event.source.getLastMouseDownTime() != latestMouseDownTime)
  {
    return;
  }
    
  auto dY = -(event.getDistanceFromDragStartY() - lastDy);
  
  if (dY == 0)
    return;
  
  const bool iOS = juce::SystemStats::getOperatingSystemType() == juce::SystemStats::OperatingSystemType::iOS;
  
  if (event.mods.isAnyModifierKeyDown() || iOS)
  {
    float iOSMultiplier = 1.0;
    
    for (int i = 1; i < mouseDownEventSources.size(); i++)
    {
      iOSMultiplier *= 10.f;
    }
      
    pixelCounter += (dY * fineSensitivity * (iOS ? iOSMultiplier : 1.0));
    auto candidate = static_cast<int>(pixelCounter);
    if (candidate >= 1 || candidate <= -1)
    {
      pixelCounter -= candidate;
      dataWheel.lock()->turn(candidate);
    }
    
  }
  else
  {
    dataWheel.lock()->turn(dY);
  }
  
  lastDy = event.getDistanceFromDragStartY();
}

static inline void clampIndex(int& dataWheelIndex)
{
  if (dataWheelIndex < 0)
  {
    while (dataWheelIndex < 0)
      dataWheelIndex += 100;
  }
  else if (dataWheelIndex > 99)
  {
    while (dataWheelIndex > 99)
      dataWheelIndex -= 100;
  }
}

void DataWheelControl::updateUI(int increment)
{
  dataWheelIndex += increment;
  clampIndex(dataWheelIndex);
  repaint();
}

void DataWheelControl::setImage(juce::Image image, int numFrames_)
{
  filmStripImage = image;
  numFrames = numFrames_;
  frameHeight = filmStripImage.getHeight() / numFrames;
  frameWidth = filmStripImage.getWidth();
  repaint();
}

DataWheelControl::~DataWheelControl()
{
  dataWheel.lock()->updateUi = [](int){};
}

void DataWheelControl::paint(juce::Graphics& g)
{
  if (filmStripImage.isValid())
  {
    int imageWidth = getWidth();
    int imageHeight = getHeight();
    
    g.drawImage(filmStripImage, 0, 0, imageWidth, imageHeight, 0, dataWheelIndex * frameHeight, frameWidth, frameHeight);
  }
}

void DataWheelControl::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
  auto dw = dataWheel.lock();
  mouseWheelControllable.processWheelEvent(wheel, [&dw](int increment) { dw->turn(increment); });
}
