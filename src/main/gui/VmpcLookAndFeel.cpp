#include "VmpcLookAndFeel.h"

using namespace juce;

AlertWindow* VmpcLookAndFeel::createAlertWindow (const String& title, const String& message,
                                      const String& button1,
                                      const String& button2,
                                      const String& button3,
                                      MessageBoxIconType iconType,
                                      int numButtons,
                                      Component* associatedComponent)
{
    AlertWindow* aw = new AlertWindow (title, message, MessageBoxIconType::NoIcon, associatedComponent);

    if (numButtons == 1)
    {
        aw->addButton (button1, 0,
                       KeyPress (KeyPress::escapeKey),
                       KeyPress (KeyPress::returnKey));
    }
    else
    {
        const KeyPress button1ShortCut ((int) CharacterFunctions::toLowerCase (button1[0]), 0, 0);
        KeyPress button2ShortCut ((int) CharacterFunctions::toLowerCase (button2[0]), 0, 0);
        if (button1ShortCut == button2ShortCut)
            button2ShortCut = KeyPress();

        if (numButtons == 2)
        {
            aw->addButton (button1, 1, KeyPress (KeyPress::returnKey), button1ShortCut);
            aw->addButton (button2, 0, KeyPress (KeyPress::escapeKey), button2ShortCut);
        }
        else if (numButtons == 3)
        {
            aw->addButton (button1, 1, button1ShortCut);
            aw->addButton (button2, 2, button2ShortCut);
            aw->addButton (button3, 0, KeyPress (KeyPress::escapeKey));
        }
    }

    rememberButton.setBounds(aw->getWidth() / 6.f, aw->getHeight(), aw->getWidth(), 40);
    rememberButton.setButtonText("Remember my preference. Change it later\nin Auto-save settings (Shift + 0, F3)");
    aw->addAndMakeVisible(&rememberButton);
    aw->setSize(aw->getWidth(), aw->getHeight() * 1.3f);
    return aw;
}
