#include "KeyboardFactory.h"
#include "Keyboard.h"
#if  defined (__APPLE__)
#include "MacOsKeyboard.h"
#elif defined (_WIN32)
#include "WindowsKeyboard.h"
#endif

Keyboard* KeyboardFactory::instance(juce::Component* parent)
{
#if  defined (__APPLE__)
  return new MacOsKeyboard(parent);
#elif defined (_WIN32)
  return new WindowsKeyboard(parent);
#else
  return new Keyboard(parent);
#endif
}
