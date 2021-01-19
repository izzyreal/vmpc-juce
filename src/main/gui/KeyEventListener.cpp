#include "KeyEventListener.h"

#include <controls/KeyEvent.hpp>
#include <controls/KeyEventHandler.hpp>

using namespace mpc::controls;
using namespace std;

KeyEventListener::KeyEventListener(weak_ptr<KeyEventHandler> keyEventHandler)
: keyEventHandler (keyEventHandler)
{
}

bool KeyEventListener::keyPressed(const juce::KeyPress &key) {
    return true;
}

bool KeyEventListener::keyEvent(const juce::KeyEvent &keyEvent) {
    DBG("KeyEventListener::keyEvent rawKeyCode " << keyEvent.rawKeyCode << (keyEvent.keyDown ? " down" : " up"));
    keyEventHandler.lock()->handle(KeyEvent(keyEvent.rawKeyCode, keyEvent.keyDown));
    return true;
}
