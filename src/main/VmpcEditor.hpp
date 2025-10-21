#pragma once

#include "gui/VmpcCornerResizerLookAndFeel.hpp"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#if __APPLE__
#include <TargetConditionals.h>
#endif

//#include "Logger.hpp"

namespace vmpc_juce::gui::vector { class View; }

namespace vmpc_juce {

class VmpcProcessor;

class VmpcEditor : public juce::AudioProcessorEditor, juce::Timer
{
public:
    explicit VmpcEditor(VmpcProcessor&);
    ~VmpcEditor() override;

    void timerCallback() override;

    bool keyPressed(const juce::KeyPress &k) override
    {
//        juce::String ch;
//        ch += k.getTextCharacter();
//        MLOG("Pressed key text description: " + k.getTextDescription().toStdString());
//        MLOG("Pressed key text character  : " + ch.toStdString());

#if __APPLE__
#if TARGET_OS_IOS
        return false;
#endif
        if (k.getTextDescription().equalsIgnoreCase("command + `") || k.getTextDescription().equalsIgnoreCase("command + Q"))
        {
            return false;
        }
#else
        if (k.getTextDescription().equalsIgnoreCase("ctrl + Q") || k.getTextDescription().equalsIgnoreCase("alt + F4") ||
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
    vmpc_juce::gui::vector::View* view = nullptr;

    VmpcCornerResizerLookAndFeel lookAndFeel;
};
}
