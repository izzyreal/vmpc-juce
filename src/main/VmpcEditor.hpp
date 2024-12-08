#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

namespace melatonin { class Inspector; }

namespace vmpc_juce {

class VmpcProcessor;

class VmpcEditor : public juce::AudioProcessorEditor
{
public:
    explicit VmpcEditor(VmpcProcessor&);
    ~VmpcEditor() override;

    void resized() override;

private:
    melatonin::Inspector* inspector = nullptr;
    VmpcProcessor &vmpcProcessor;
    const float initial_width =  445.f;
    const float initial_height = 342.f;
    const float initial_scale = (2.0f - 0.1f) * 1.2f;

    juce::Font nimbusSans;
};
}
