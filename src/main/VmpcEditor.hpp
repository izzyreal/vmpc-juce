#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

#include "gui/VmpcNoCornerResizerLookAndFeel.hpp"

namespace vmpc_juce::gui::vector { class View; }
namespace melatonin { class Inspector; }

namespace vmpc_juce {

class VmpcProcessor;

class VmpcEditor : public juce::AudioProcessorEditor
{
public:
    explicit VmpcEditor(VmpcProcessor&);
    ~VmpcEditor() override;

    void resized() override;

    static const int initial_width =  445;
    static const int initial_height = 342;

private:
    melatonin::Inspector* inspector = nullptr;
    VmpcProcessor &vmpcProcessor;
    vmpc_juce::gui::vector::View* view = nullptr;
    const float initial_scale = 1.31f;

    juce::Font *nimbusSans = nullptr;
    juce::Font *mpc2000xlFaceplateGlyphs = nullptr;
    VmpcNoCornerResizerLookAndFeel lookAndFeel;
};
}
