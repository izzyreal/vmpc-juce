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

bool KeyEventListener::keyPressed(const juce::KeyPress& k)
{
  if (k.getTextDescription().toStdString() == "command + Q")
    return false;
  
  return true;
}

bool KeyEventListener::keyEvent(const juce::KeyEvent &keyEvent)
{
  keyEventHandler.lock()->handle(KeyEvent(keyEvent.rawKeyCode, keyEvent.keyDown));
  return true;
}

void KeyEventListener::mouseDown(const juce::MouseEvent& e) {
  bool exists = false;
  for (auto& s : sources) {
    if (s->getIndex() == e.source.getIndex()) { exists = true; break; }
  }
  if (!exists) sources.push_back(std::make_shared<juce::MouseInputSource>(e.source));

  auto pos1 = sources[0]->getLastMouseDownPosition();
  
  if (sources.size() == 1) {
    prevSingleX = pos1.getX();
    prevSingleY = pos1.getY();
  } else if (sources.size() == 2) {
    auto pos2 = sources[1]->getLastMouseDownPosition();
    float length = juce::Line<int>(pos1.getX(), pos1.getY(), pos2.getX(), pos2.getY()).getLength();
    prevDistance = length;
  }
}

void KeyEventListener::mouseUp(const juce::MouseEvent& e)
{
  for (int i = 0; i < sources.size(); i++) {
    if (sources[i]->getIndex() == e.source.getIndex()) {
      sources.erase(sources.begin() + i);
      prevDistance = -1.f;
      prevPinchCx = -1.f;
      prevPinchCy = -1.f;
      break;
    }
  }
}

void KeyEventListener::mouseDrag(const juce::MouseEvent& e) {

  auto thisSource = e.source;
  auto cur_pos1 = thisSource.getScreenPosition();
  auto content = getParentComponent();

  if (sources.size() == 1) {
    auto translation_x = prevSingleX != -1.f ? (cur_pos1.getX() - prevSingleX) : 0.f;
    auto translation_y = prevSingleY != -1.f ? (cur_pos1.getY() - prevSingleY) : 0.f;
    prevSingleX = cur_pos1.getX();
    prevSingleY = cur_pos1.getY();
    auto newX = content->getX() + translation_x;
    auto newY = content->getY() + translation_y;
    content->setBounds(newX, newY, content->getWidth(), content->getHeight());
  }
  else if (sources.size() == 2) {
    
    // For smooth transitio between 1 and 2 fingers:
    // We should keep track of prevxy in a map per e.source.getIndex
    prevSingleX = -1.f;
    prevSingleY = -1.f;
    
    std::shared_ptr<juce::MouseInputSource> thatSource;
    
    for (auto& s : sources) {
      if (s->getIndex() != thisSource.getIndex()) {
        thatSource = s;
        break;
      }
    }
    
    auto cur_pos2 = thatSource->getScreenPosition();
    
    float cur_distance = juce::Line<int>(cur_pos1.getX(), cur_pos1.getY(), cur_pos2.getX(), cur_pos2.getY()).getLength();
    
    auto scale = prevDistance != -1 ? (cur_distance / prevDistance) : 1.0f;
    prevDistance = cur_distance;
    auto new_w = content->getWidth() * scale;
    auto ratio = 1298.0 / 994.0;
    auto new_h = new_w / ratio;

    auto contentScale = content->getWidth() / 1298.0;
    auto screen_pinch_cx = (cur_pos1.getX() + cur_pos2.getX()) / 2;
    auto current_x = content->getX();
    auto current_content_pinch_cx = (screen_pinch_cx * contentScale) - current_x;
    auto new_content_pinch_cx = current_content_pinch_cx * scale;
    auto new_x = (screen_pinch_cx * contentScale) - new_content_pinch_cx;
    
    auto screen_pinch_cy = (cur_pos1.getY() + cur_pos2.getY()) / 2;
    auto current_y = content->getY();
    auto current_content_pinch_cy = (screen_pinch_cy * contentScale) - current_y;
    auto new_content_pinch_cy = current_content_pinch_cy * scale;
    auto new_y = (screen_pinch_cy * contentScale) - new_content_pinch_cy;

    auto translation_x = prevPinchCx != -1.f ? (screen_pinch_cx - prevPinchCx) : 0.f;
    auto translation_y = prevPinchCy != -1.f ? (screen_pinch_cy - prevPinchCy) : 0.f;
    
    translation_x *= 1.2f;
    translation_y *= 1.2f;
    
    new_x += translation_x;
    new_y += translation_y;
    
    prevPinchCx = screen_pinch_cx;
    prevPinchCy = screen_pinch_cy;

    auto primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    
    if (primaryDisplay != nullptr) {
      auto area = primaryDisplay->userArea;
      auto portrait = area.getWidth() < area.getHeight();
      
      if (portrait && new_h < area.getHeight()) {
        return;
      }
      else if (!portrait && new_w < area.getWidth()) {
        return;
      }
      else if (portrait && new_h > area.getHeight() * 2) {
        return;
      }
      else if (!portrait && new_w > area.getWidth() * 2) {
        return;
      }
      
      content->setBounds(new_x, new_y, new_w, new_h);
    }
  }
}


