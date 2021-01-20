#include "Led.hpp"

#include <string>

using namespace juce;
using namespace std;

Led::Led(Image _led, Rectangle<float> _rect)
:led (_led), rect (_rect)
{
}

void Led::setBounds()
{
    Component::setBounds(rect.getX(), rect.getY(), led.getWidth(), led.getHeight());
}

void Led::paint(Graphics& g)
{
    if (on)
        g.drawImage(led, 0, 0, led.getWidth(), led.getHeight(), 0, 0, led.getWidth(), led.getHeight());
}

void Led::setOn(bool b)
{
    if (on == b)
        return;
    
    const MessageManagerLock mml(Thread::getCurrentThread());
    
    if (!mml.lockWasGained())
        return;
    
    on = b;
    repaint();
}
