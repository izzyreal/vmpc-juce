#include "MacOsKeyboard.h"
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

void Foobaru::foo() {
  [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                        handler:^NSEvent*(NSEvent* event)
  {
    auto c = [event keyCode];
    printf("\nnote %d\n", c);
//      handleRawKeyEvent(KeyEvent([event keyCode], true));
      return event;
  }];
}
