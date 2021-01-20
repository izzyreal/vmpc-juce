#pragma once
#include "VmpcComponent.h"

#include <memory>

#include <observer/Observer.hpp>

namespace mpc::hardware {
class Slider;
}

class SliderControl
: public VmpcComponent, public moduru::observer::Observer
{
    
private:
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
    
    void update(moduru::observer::Observable* o, nonstd::any arg) override;
    
    SliderControl(std::weak_ptr<mpc::hardware::Slider> slider, int startIndex);
    
    ~SliderControl() override;
    
};
