#pragma once
#include "VmpcComponent.hpp"

#include "MouseWheelControllable.hpp"

#include <memory>

namespace mpc::hardware {
class Pot;
}

class KnobControl
: public VmpcComponent
{
    
private:
    MouseWheelControllable mouseWheelControllable;
    std::weak_ptr<mpc::hardware::Pot> pot;

public:
    void paint(juce::Graphics& g) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

public:
    void setImage(juce::Image image);
    
private:
    juce::Image knobs;
    int frameWidth, frameHeight, lastDy = 0;
    
public:
    KnobControl(std::weak_ptr<mpc::hardware::Pot> pot);
    
};
