#include "KeyboardFactory.h"
#include "Keyboard.h"
#if  defined (__APPLE__)
#include "MacOsKeyboard.h"
#elif defined (_WIN32)
#include "WindowsKeyboard.h"
#endif

Keyboard* KeyboardFactory::instance()
{
#if  defined (__APPLE__)
  return new MacOsKeyboard();
#elif defined (_WIN32)
  return new WindowsKeyboard();
#else
  return new Keyboard();
#endif
}
