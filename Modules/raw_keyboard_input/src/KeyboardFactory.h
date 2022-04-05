#pragma once

class Keyboard;
class KeyboardFactory {
public:
  static Keyboard* instance();
};
