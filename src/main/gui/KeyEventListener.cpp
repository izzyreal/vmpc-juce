#include "KeyEventListener.h"

#include <Mpc.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/LayeredScreen.hpp>
#include <lcdgui/screens/VmpcKeyboardScreen.hpp>

#include <controls/KeyEvent.hpp>
#include <controls/KeyEventHandler.hpp>

using namespace mpc::controls;
using namespace mpc::lcdgui::screens;
using namespace std;

KeyEventListener::KeyEventListener(mpc::Mpc& _mpc)
: mpc (_mpc), keyEventHandler (mpc.getControls().lock()->getKeyEventHandler())
{
}

bool KeyEventListener::keyPressed(const juce::KeyPress &key)
{
    if (mpc.getLayeredScreen().lock()->getCurrentScreenName().compare("vmpc-keyboard") != 0)
    {
        return true;
    }

    auto screen = mpc.screens->get<VmpcKeyboardScreen>("vmpc-keyboard");
    auto desc = key.getTextDescription();
    
    if (screen->isLearning())
        return true;
    
    if (desc.equalsIgnoreCase("cursor up"))
    {
        screen->up();
        return true;
    }
    else if (desc.equalsIgnoreCase("cursor down"))
    {
        screen->down();
        return true;
    }
    else if (desc.equalsIgnoreCase("f1"))
    {
        screen->function(0);
        return true;
    }
    else if (desc.equalsIgnoreCase("f2"))
    {
        screen->function(1);
        return true;
    }
    else if (desc.equalsIgnoreCase("f3"))
    {
        screen->function(2);
        return true;
    }
    else if (desc.equalsIgnoreCase("f4"))
    {
        screen->function(3);
        return true;
    }
    else if (desc.equalsIgnoreCase("f5"))
    {
        screen->function(4);
        return true;
    }
    else if (desc.equalsIgnoreCase("f6"))
    {
        screen->function(5);
        return true;
    }
    else if (desc.equalsIgnoreCase("escape"))
    {
        screen->mainScreen();
        return true;
    }
    
    return false;
}

bool KeyEventListener::keyEvent(const juce::KeyEvent &keyEvent)
{
    keyEventHandler.lock()->handle(KeyEvent(keyEvent.rawKeyCode, keyEvent.keyDown));
    return true;
}
