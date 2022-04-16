#include "LinuxKeyboard.h"

#include <juce_gui_basics/native/x11/juce_linux_XWindowSystem.h>

LinuxKeyboard::LinuxKeyboard(juce::Component* parent) : Keyboard(parent)
{
    eventLoop = new std::thread([&](){
        while (true) {
        if (display == nullptr)
        {
            auto xWindowSystem = juce::XWindowSystem::getInstanceWithoutCreating();
            display = xWindowSystem->getDisplay();
        }

        if (display != nullptr)
        {
            char keymap[32];
            XQueryKeymap(display, keymap);
            static unsigned int masktable[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

            for (int i = 0; i < 256; i++) {
                bool wasDown = prev_keymap[i >> 3] & masktable[i & 7];
                bool isDown = keymap[i >> 3] & masktable[i & 7];
                KeySym ks = XKeycodeToKeysym(display, (KeyCode) i, 0);
                if (isDown && !wasDown) {
                    addPressedKey(ks);
                } else if (!isDown && wasDown) {
                    removePresedKey(ks);
                }
            }

            for (int i = 0; i < 32; i++)
                prev_keymap[i] = keymap[i];
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });
}

void LinuxKeyboard::timerCallback()
{
    Keyboard::timerCallback();
}

LinuxKeyboard::~LinuxKeyboard()
{
  // remove hook
}
