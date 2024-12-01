#include "Led.hpp"

Led::Led(juce::Image _led, juce::Rectangle<float> _rect)
:led (_led), rect (_rect)
{
}

void Led::setBounds()
{
    Component::setBounds(rect.getX(), rect.getY(), led.getWidth(), led.getHeight());
}

void Led::paint(juce::Graphics& g)
{
    if (on)
        g.drawImage(led, 0, 0, led.getWidth(), led.getHeight(), 0, 0, led.getWidth(), led.getHeight());
}

void Led::setOn(bool b)
{
    if (on == b)
        return;
    
    const juce::MessageManagerLock mml(juce::Thread::getCurrentThread());
    
    if (!mml.lockWasGained())
        return;
    
    on = b;
    repaint();
}
