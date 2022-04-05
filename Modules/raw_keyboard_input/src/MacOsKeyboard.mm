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
  keyDownMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                                         handler:^NSEvent*(NSEvent* event) {
    addPressedKey([event keyCode]);
    return event;
  }];
  
  keyUpMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp
                                                       handler:^NSEvent*(NSEvent* event) {
    removePresedKey([event keyCode]);
    return event;
  }];
  
  modifierChangedMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskFlagsChanged
                                                                 handler:^NSEvent*(NSEvent* event) {
    auto code = [event keyCode];
    
    if (code == 0x3A || code == 0x3D) {
      if ([event modifierFlags] & NSEventModifierFlagOption) {
        addPressedKey(code);
      } else {
        removePresedKey(code);
      }
    }
    else if (code == 0x38 || code == 0x3C) {
      if ([event modifierFlags] & NSEventModifierFlagShift) {
        addPressedKey(code);
      } else {
        removePresedKey(code);
      }
    }
    else if (code == 0x3B || code == 0x3E) {
      if ([event modifierFlags] & NSEventModifierFlagControl) {
        addPressedKey(code);
      } else {
        removePresedKey(code);
      }
    }
    else if (code == 0x36 || code == 0x37) {
      if ([event modifierFlags] & NSEventModifierFlagCommand) {
        addPressedKey(code);
      } else {
        removePresedKey(code);
      }
    }
    else if (code == 0x3F) {
      if ([event modifierFlags] & NSEventModifierFlagFunction) {
        addPressedKey(code);
      } else {
        removePresedKey(code);
      }
    }
    else if (code == 0x39) {
      if ([event modifierFlags] & NSEventModifierFlagCapsLock) {
        addPressedKey(code);
      } else {
        removePresedKey(code);
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
