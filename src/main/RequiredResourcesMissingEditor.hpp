#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce
{
    class VmpcProcessor;

    class RequiredResourcesMissingEditor final
        : public juce::AudioProcessorEditor
    {
    public:
        RequiredResourcesMissingEditor(VmpcProcessor &processor,
                                       const std::string &message);

        void resized() override;

    private:
        juce::TextEditor text;
    };
} // namespace vmpc_juce
