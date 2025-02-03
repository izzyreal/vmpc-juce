#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

#include "gui/VmpcCornerResizerLookAndFeel.hpp"
#include "juce_gui_basics/juce_gui_basics.h"

namespace vmpc_juce::gui::vector { class View; }
namespace melatonin { class Inspector; }

namespace vmpc_juce {

class VmpcProcessor;

class VmpcEditor : public juce::AudioProcessorEditor
{
public:
    explicit VmpcEditor(VmpcProcessor&);
    ~VmpcEditor() override;

    bool keyPressed(const juce::KeyPress &) override { return !juce::JUCEApplication::isStandaloneApp(); }

    void resized() override;

private:
    melatonin::Inspector* inspector = nullptr;
    VmpcProcessor &vmpcProcessor;
    vmpc_juce::gui::vector::View* view = nullptr;

    VmpcCornerResizerLookAndFeel lookAndFeel;
};
}
