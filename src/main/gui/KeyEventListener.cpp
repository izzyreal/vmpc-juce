#include "KeyEventListener.h"

#include <Mpc.hpp>

#include <controls/Controls.hpp>
#include <controls/KeyEvent.hpp>
#include <controls/KeyEventHandler.hpp>

using namespace mpc::controls;
using namespace std;

KeyEventListener::KeyEventListener(mpc::Mpc& _mpc)
: mpc (_mpc), keyEventHandler (mpc.getControls().lock()->getKeyEventHandler())
{
    setName("KeyEventListener");
}

bool KeyEventListener::keyPressed(const juce::KeyPress&)
{
    return true;
}

bool KeyEventListener::keyEvent(const juce::KeyEvent &keyEvent)
{
    keyEventHandler.lock()->handle(KeyEvent(keyEvent.rawKeyCode, keyEvent.keyDown));
    return true;
}
