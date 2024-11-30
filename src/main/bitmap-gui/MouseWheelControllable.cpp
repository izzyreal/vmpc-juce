#include "MouseWheelControllable.hpp"

void MouseWheelControllable::processWheelEvent(const juce::MouseWheelDetails& details,
                                                       std::function<void(int)> updateFn)
{
    auto y = -details.deltaY;
    
    acc += y;
    
    auto increment = 0;
    
    if (acc > THRESHOLD)
    {
        increment = 1;
        
        while (acc > THRESHOLD)
            acc -= THRESHOLD;
    }
    else if (abs(acc) > THRESHOLD)
    {
        increment = -1;
        
        while (abs(acc) > THRESHOLD)
            acc += THRESHOLD;
    }

    if (increment == 0) return;
    
    updateFn(increment);
}
