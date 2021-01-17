#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class KeyEventListener : public juce::Component {
public:
    bool keyPressed(const juce::KeyPress &key) override;
    bool keyEvent(const juce::KeyEvent &keyEvent) override;
};
