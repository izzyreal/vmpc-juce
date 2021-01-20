#pragma once
#include "VmpcComponent.h"

class Led
: public VmpcComponent
{
    
private:
    juce::Image led;
    
    juce::Rectangle<float> rect;
    bool on = false;
    
public:
    void setOn(bool b);
    void setBounds();
    
public:
    void paint(juce::Graphics& g) override;
    
public:
    Led(juce::Image led, juce::Rectangle<float> rect);
    
};
