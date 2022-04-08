#include "MacOsKeyboard.h"
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

MacOsKeyboard::MacOsKeyboard()
{
  installMonitor();
}

MacOsKeyboard::~MacOsKeyboard()
{
  removeMonitor();
}

void MacOsKeyboard::installMonitor()
{
  if (thisses.size() > 1)
    return;
  
  keyDownMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                                         handler:^NSEvent*(NSEvent* event) {
    processKeyEvent([event keyCode], true);
    return event;
  }];
  
  keyUpMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp
                                                       handler:^NSEvent*(NSEvent* event) {
    processKeyEvent([event keyCode], false);
    return event;
  }];
  
  modifierChangedMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskFlagsChanged
                                                                 handler:^NSEvent*(NSEvent* event) {
    auto code = [event keyCode];
    
    if (code == 0x3A || code == 0x3D) {
      if ([event modifierFlags] & NSEventModifierFlagOption) {
        processKeyEvent(code, true);
      } else {
        processKeyEvent(code, false);
      }
    }
    else if (code == 0x38 || code == 0x3C) {
      if ([event modifierFlags] & NSEventModifierFlagShift) {
        processKeyEvent(code, true);
      } else {
        processKeyEvent(code, false);
      }
    }
    else if (code == 0x3B || code == 0x3E) {
      if ([event modifierFlags] & NSEventModifierFlagControl) {
        processKeyEvent(code, true);
      } else {
        processKeyEvent(code, false);
      }
    }
    else if (code == 0x36 || code == 0x37) {
      if ([event modifierFlags] & NSEventModifierFlagCommand) {
        processKeyEvent(code, true);
      } else {
        processKeyEvent(code, false);
      }
    }
    else if (code == 0x3F) {
      if ([event modifierFlags] & NSEventModifierFlagFunction) {
        processKeyEvent(code, true);
      } else {
        processKeyEvent(code, false);
      }
    }
    else if (code == 0x39) {
      if ([event modifierFlags] & NSEventModifierFlagCapsLock) {
        processKeyEvent(code, true);
      } else {
        processKeyEvent(code, false);
      }
    }
    
    return event;
  }];
  
}

void MacOsKeyboard::removeMonitor()
{
  if (keyDownMonitor != nullptr) {
    [NSEvent removeMonitor:(id)keyDownMonitor];
    keyDownMonitor = nullptr;
  }
  if (keyUpMonitor != nullptr) {
    [NSEvent removeMonitor:(id)keyUpMonitor];
    keyUpMonitor =  nullptr;
  }
  if (modifierChangedMonitor != nullptr) {
    [NSEvent removeMonitor:(id)modifierChangedMonitor];
    modifierChangedMonitor = nullptr;
  }
}
