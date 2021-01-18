#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace mpc::controls {
class KeyEventHandler;
}

class KeyEventListener : public juce::Component {
public:
    KeyEventListener(std::weak_ptr<mpc::controls::KeyEventHandler> keyEventHandler);
    bool keyPressed(const juce::KeyPress &key) override;
    bool keyEvent(const juce::KeyEvent &keyEvent) override;
    
private:
    std::weak_ptr<mpc::controls::KeyEventHandler> keyEventHandler;
};
