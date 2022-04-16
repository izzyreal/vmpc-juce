#include "Keyboard.h"

std::set<Keyboard*> Keyboard::thisses;

Keyboard::Keyboard(juce::Component* parent) : parent(parent)
{
  thisses.emplace(this);
  startTimer(1);
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
    if (peer == nullptr) {
        auto _peer = parent->getPeer();
        if (_peer != nullptr) {
            peer = _peer;
            stopTimer();
        }
    }
}

bool Keyboard::isKeyDown(int keyCode)
{
  std::lock_guard<std::recursive_mutex> lock(pressedKeysMutex);
  return pressedKeys.count(keyCode) == 1;
}

void Keyboard::addPressedKey(int keyCode)
{
  std::lock_guard<std::recursive_mutex> lock(pressedKeysMutex);
  pressedKeys.emplace(keyCode);
  if (onKeyDownFn) onKeyDownFn(keyCode);
}

void Keyboard::removePresedKey(int keyCode)
{
  std::lock_guard<std::recursive_mutex> lock(pressedKeysMutex);

  if (pressedKeys.count(keyCode) == 1)
    pressedKeys.erase(keyCode);

  if (onKeyUpFn) onKeyUpFn(keyCode);
}

void Keyboard::allKeysUp()
{
  std::lock_guard<std::recursive_mutex> lock(pressedKeysMutex);

  for (auto keyCode : pressedKeys)
    onKeyUpFn(keyCode);

  pressedKeys.clear();
}
