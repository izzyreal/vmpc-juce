#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace mpc { class Mpc; }

namespace mpc::controls {
class KeyEventHandler;
}

class KeyEventListener
: public juce::Component
{
public:
    KeyEventListener(mpc::Mpc&);
    bool keyPressed(const juce::KeyPress &key) override;
    bool keyEvent(const juce::KeyEvent &keyEvent) override;
    
private:
    mpc::Mpc& mpc;
    std::weak_ptr<mpc::controls::KeyEventHandler> keyEventHandler;
};
