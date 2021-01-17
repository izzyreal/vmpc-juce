#include "KeyEventListener.h"

bool KeyEventListener::keyPressed(const juce::KeyPress &key) {
//    DBG("MyListener::keyPressed");
    return true;
}

bool KeyEventListener::keyEvent(const juce::KeyEvent &keyEvent) {
    DBG("MyListener::keyEvent rawKeyCode " << keyEvent.rawKeyCode << (keyEvent.keyDown ? " down" : " up"));
    return false;
}
