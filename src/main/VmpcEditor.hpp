#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

namespace vmpc_juce {

class VmpcProcessor;

class VmpcEditor : public juce::AudioProcessorEditor
{
public:
    explicit VmpcEditor(VmpcProcessor&);

private:
    VmpcProcessor &vmpcProcessor;
};
}
