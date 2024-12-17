/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
#include "SliderControl.hpp"
#include <hardware/HwSlider.hpp>

using namespace vmpc_juce::gui::bitmap;

static inline void clampIndex(int& sliderIndex)
{
    if (sliderIndex < 0)
        sliderIndex = 0;
    else if (sliderIndex > 99)
        sliderIndex = 99;
}

SliderControl::SliderControl(std::weak_ptr<mpc::hardware::Slider> _slider)
: slider (_slider), sliderIndex (static_cast<int>(_slider.lock()->getValue() / 1.27))
{
    clampIndex(sliderIndex);
}

void SliderControl::mouseUp(const juce::MouseEvent& event)
{
    lastDy = 0;
    Component::mouseUp(event);
}

void SliderControl::mouseDrag(const juce::MouseEvent& event)
{
    auto dY = -(event.getDistanceFromDragStartY() - lastDy);
    lastDy = event.getDistanceFromDragStartY();
    slider.lock()->setValue(slider.lock()->getValue() + dY);
    sliderIndex = 100 - static_cast<int>(slider.lock()->getValue() / 1.27);
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
    auto candidateSliderIndex = 100 - static_cast<int>(newValue / 1.27);
    
    if (candidateSliderIndex == sliderIndex)
        return;
    
    sliderIndex = candidateSliderIndex;
    clampIndex(sliderIndex);
    repaint();
}

void SliderControl::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    auto s = slider.lock();
    mouseWheelControllable.processWheelEvent(wheel, [&](int increment) {     s->setValue(s->getValue() + increment);
        sliderIndex = 100 - static_cast<int>(s->getValue() / 1.27);
        clampIndex(sliderIndex);
        repaint();
    });
}
