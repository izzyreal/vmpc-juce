#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class VmpcLookAndFeel : public juce::LookAndFeel_V4 {

public:
    juce::AlertWindow* createAlertWindow (const juce::String& title, const juce::String& message,
                                            const juce::String& button1,
                                            const juce::String& button2,
                                            const juce::String& button3,
                                            juce::MessageBoxIconType iconType,
                                            int numButtons,
                                            juce::Component* associatedComponent) override;

    juce::ToggleButton rememberButton;
};