#pragma once

#include "gui/VmpcCornerResizerLookAndFeel.hpp"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#if __APPLE__
#include <TargetConditionals.h>
#endif

// #include "Logger.hpp"

namespace vmpc_juce::gui::vector
{
    class View;
}

namespace vmpc_juce
{

    class VmpcProcessor;

    class VmpcEditor : public juce::AudioProcessorEditor,
                       juce::Timer,
                       public juce::RawKeyEventSink
    {
    public:
        explicit VmpcEditor(VmpcProcessor &);
        ~VmpcEditor() override;

        void timerCallback() override;

        // Invoked by patched JUCE code (raw_keyboard.patch), but only
        // on macOS. See juce_NSViewComponentPeer.mm, juce_RawKeyEvent.hpp, etc.
        // We usually rely on juce-raw-keyboard-input-module, which, on
        // macOS, sets up an event monitor in the current process. This works
        // well in most hosts and most platforms, but in Renoise on macOS it
        // doesn't, except for modifier input. So it seems that Renoise is
        // (knowingly or unknowingly) preventing non-modifier monitoring.
        // So in this case we rely on juce_NSViewComponentPeer.mm propagating
        // raw key events, which it only does for non-modifier events.
        // juce-raw-keyboard-input-module is still used in Renoise on macOS
        // for modifier events.
        // Also adding to the list: Ardour on macOS.
        void handleRawKeyEvent(const juce::RawKeyEvent &) override;

        bool keyPressed(const juce::KeyPress &k) override
        {
            //        juce::String ch;
            //        ch += k.getTextCharacter();
            //        MLOG("Pressed key text description: " +
            //        k.getTextDescription().toStdString()); MLOG("Pressed key
            //        text character  : " + ch.toStdString());

#if __APPLE__
#if TARGET_OS_IOS
            return false;
#endif
            if (k.getTextDescription().equalsIgnoreCase("command + `") ||
                k.getTextDescription().equalsIgnoreCase("command + Q"))
            {
                return false;
            }
#else
            if (k.getTextDescription().equalsIgnoreCase("ctrl + Q") ||
                k.getTextDescription().equalsIgnoreCase("alt + F4") ||
                k.getTextDescription().equalsIgnoreCase("ctrl + tab"))
            {
                return false;
            }
#endif
            return true;
        }

        void resized() override;

    private:
        VmpcProcessor &vmpcProcessor;
        vmpc_juce::gui::vector::View *view = nullptr;

        VmpcCornerResizerLookAndFeel lookAndFeel;
    };
} // namespace vmpc_juce
