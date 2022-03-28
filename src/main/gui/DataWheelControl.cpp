#include "DataWheelControl.h"

#include <Logger.hpp>

DataWheelControl::DataWheelControl(std::weak_ptr<mpc::hardware::DataWheel> _dataWheel)
:	numFrames(0), frameWidth(0), frameHeight(0), dataWheel (_dataWheel)
{
}

void DataWheelControl::mouseUp(const juce::MouseEvent& event)
{
    lastDy = 0;
    getParentComponent()->mouseUp(event);
}

void DataWheelControl::mouseDrag(const juce::MouseEvent& event)
{
    auto dY = -(event.getDistanceFromDragStartY() - lastDy);
    
    if (dY == 0)
        return;
    
    if (event.mods.isAnyModifierKeyDown())
    {
        pixelCounter += (dY * fineSensitivity);
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

void DataWheelControl::update(moduru::observer::Observable*, nonstd::any arg)
{
    int increment = nonstd::any_cast<int>(arg);
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
    dataWheel.lock()->deleteObserver(this);
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
