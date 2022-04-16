#include "KeyboardFactory.h"
#include "Keyboard.h"
#if  defined (__APPLE__)
#include "MacOsKeyboard.h"
#elif defined (_WIN32)
#include "WindowsKeyboard.h"
#elif defined (__linux__)
#include "LinuxKeyboard.h"
#endif

Keyboard* KeyboardFactory::instance(juce::Component* parent)
{
#if  defined (__APPLE__)
  return new MacOsKeyboard(parent);
#elif defined (_WIN32)
  return new WindowsKeyboard(parent);
#elif defined (__linux__)
  return new LinuxKeyboard(parent);
#else
  return new Keyboard(parent);
#endif
}
