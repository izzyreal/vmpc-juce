#include "SliderControl.hpp"
#include <hardware/HwSlider.hpp>

#include <Logger.hpp>

static inline void clampIndex(int& sliderIndex)
{
    if (sliderIndex < 0)
        sliderIndex = 0;
    else if (sliderIndex > 99)
        sliderIndex = 99;
}

SliderControl::SliderControl(std::weak_ptr<mpc::hardware::Slider> _slider, int startIndex)
: slider (_slider)
{
    sliderIndex = startIndex;
    sliderIndex = static_cast<int>(slider.lock()->getValue() / 1.27);
    clampIndex(sliderIndex);
}

void SliderControl::mouseUp(const juce::MouseEvent& event)
{
    lastDy = 0;
    Component::mouseUp(event);
}

void SliderControl::mouseDrag(const juce::MouseEvent& event)
{
    auto dY = event.getDistanceFromDragStartY() - lastDy;
    lastDy = event.getDistanceFromDragStartY();
    slider.lock()->setValue(slider.lock()->getValue() + dY);
    sliderIndex = static_cast<int>(slider.lock()->getValue() / 1.27);
    clampIndex(sliderIndex);
    repaint();
}

void SliderControl::setImage(juce::Image image)
{
    filmStripImage = image;
    frameHeight = filmStripImage.getHeight() / 100;
    frameWidth = filmStripImage.getWidth();
    repaint();
}

void SliderControl::paint(juce::Graphics& g)
{
    if (filmStripImage.isValid())
    {
        int imageWidth = getWidth();
        int imageHeight = getHeight();

        g.drawImage(filmStripImage, 0, 0, imageWidth, imageHeight, 0, sliderIndex * frameHeight, frameWidth, frameHeight);
    }
}

void SliderControl::timerCallback()
{
    auto newValue = slider.lock()->getValue();
    auto candidateSliderIndex = static_cast<int>(newValue / 1.27);
    
    if (candidateSliderIndex == sliderIndex)
        return;
    
    sliderIndex = candidateSliderIndex;
    clampIndex(sliderIndex);
    repaint();
}
