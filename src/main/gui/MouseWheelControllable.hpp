#pragma once
#include "juce_gui_basics/juce_gui_basics.h"

#include <functional>

namespace vmpc_juce::gui {
class MouseWheelControllable {
private:
    const double THRESHOLD = 0.1;
    double acc = 0.0;
    
public:
    void processWheelEvent(const juce::MouseWheelDetails&,
                           std::function<void(int)> updateFn);

};
} // namespace vmpc_juce::gui
