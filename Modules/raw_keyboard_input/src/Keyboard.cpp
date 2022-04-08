#include "Keyboard.h"

std::set<Keyboard*> Keyboard::thisses;

Keyboard::Keyboard()
{
  thisses.emplace(this);
  startTimer(10);
}

Keyboard::~Keyboard()
{
  thisses.erase(this);
}

bool Keyboard::processKeyEvent(int keyCode, bool isKeyDown)
{
  auto focusedPeer = getFocusedPeer();
    
  if (focusedPeer == nullptr)
    return false;
  
  for (auto t : thisses) {
    if (t->peer == focusedPeer) {
      if (isKeyDown)
        t->addPressedKey(keyCode);
      else
        t->removePresedKey(keyCode);
    }
  }
  
  return true;
}

juce::ComponentPeer* Keyboard::getFocusedPeer()
{
  for (int i = 0; i < juce::ComponentPeer::getNumPeers(); i++) {
    if (juce::ComponentPeer::getPeer(i)->isFocused()) {
      return juce::ComponentPeer::getPeer(i);
    }
  }
  return nullptr;
}

void Keyboard::timerCallback()
{
  auto _peer = getPeer();
  if (_peer != nullptr) {
    peer = _peer;
    stopTimer();
  }
}

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

void Keyboard::allKeysUp()
{
  for (auto keyCode : pressedKeys)
    onKeyUpFn(keyCode);
  
  pressedKeys.clear();
}
