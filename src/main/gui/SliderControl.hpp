#pragma once
#include "VmpcComponent.hpp"

#include "MouseWheelControllable.hpp"

#include <memory>

namespace mpc::hardware {
class Slider;
}

class SliderControl
: public VmpcComponent, public juce::Timer
{
    
private:
    MouseWheelControllable mouseWheelControllable;
    std::weak_ptr<mpc::hardware::Slider> slider;
    int sliderIndex{ 0 };
    
private:
    juce::Image filmStripImage;
    int frameWidth, frameHeight;
    int lastDy = 0;
    
public:
    void setImage(juce::Image image);
    
public:
    void paint(juce::Graphics& g) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    void timerCallback() override;
    
    SliderControl(std::weak_ptr<mpc::hardware::Slider> slider);
};
