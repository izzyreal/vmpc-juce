#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "VmpcTooltipComponent.hpp"

class KeyboardButton : public juce::ImageButton {
public:
    void mouseEnter(const juce::MouseEvent&) override
    {
        for (auto& c1 : getParentComponent()->getParentComponent()->getChildren())
        {
            if (auto c2 = dynamic_cast<VmpcTooltipComponent*>(c1))
            {
                c2->showKeyboardMapping();
            }
        }
    }
    
    void mouseExit(const juce::MouseEvent& e) override
    {
        juce::ImageButton::mouseExit(e);
        
        for (auto& c1 : getParentComponent()->getParentComponent()->getChildren())
        {
            if (auto c2 = dynamic_cast<VmpcTooltipComponent*>(c1))
            {
                c2->hideKeyboardMapping();
            }
        }
    }
};
