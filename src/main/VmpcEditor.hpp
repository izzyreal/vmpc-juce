#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

#include "gui/VmpcNoCornerResizerLookAndFeel.hpp"
#include "juce_core/juce_core.h"

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

    const juce::String typefaceName = "Neutral Sans";
    const juce::String typefaceStyle = "Bold";
    juce::ReferenceCountedObjectPtr<juce::Typeface> nimbusSansTypeface;
    juce::Font nimbusSans;
    juce::Font nimbusSans2;
    juce::Font mpc2000xlFaceplateGlyphs;
    VmpcNoCornerResizerLookAndFeel lookAndFeel;
    juce::LookAndFeel *globalFontLaf = nullptr;
};
}
