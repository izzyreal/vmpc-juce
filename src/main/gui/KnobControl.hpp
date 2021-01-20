#pragma once
#include "VmpcComponent.h"

#include <memory>

namespace mpc::hardware {
class Pot;
}

class KnobControl
: public VmpcComponent
{
    
private:
    std::weak_ptr<mpc::hardware::Pot> pot;
    int knobIndex = 0;
    int knobType = 0; // 0 = rec, 1 = vol
    
public:
    void paint(juce::Graphics& g) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    
public:
    void setImage(juce::Image image);
    
private:
    juce::Image knobs;
    int frameWidth, frameHeight, lastDy = 0;
    
public:
    KnobControl(int type, std::weak_ptr<mpc::hardware::Pot> pot, int startIndex);
    
};
