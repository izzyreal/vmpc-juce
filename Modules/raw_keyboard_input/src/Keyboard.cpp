#include "Keyboard.h"

bool Keyboard::isKeyDown(int keyCode)
{
  return pressedKeys.count(keyCode) == 1;
}

void Keyboard::addPressedKey(int keyCode)
{
  pressedKeys.emplace(keyCode);
  if (onKeyDownFn) onKeyDownFn(keyCode);
}

void Keyboard::removePresedKey(int keyCode)
{
  if (isKeyDown(keyCode)) pressedKeys.erase(keyCode);
  if (onKeyUpFn) onKeyUpFn(keyCode);
}
