#pragma once
#include "juce_gui_basics/juce_gui_basics.h"

#include <functional>

class MouseWheelControllable {
private:
    const double THRESHOLD = 0.1;
    double acc = 0.0;
    
public:
    void processWheelEvent(const juce::MouseWheelDetails&,
                           std::function<void(int)> updateFn);

};
