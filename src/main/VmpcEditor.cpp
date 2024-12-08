#include "VmpcEditor.hpp"
#include "VmpcProcessor.hpp"

#include "VmpcJuceResourceUtil.hpp"

using namespace vmpc_juce;

VmpcEditor::VmpcEditor(VmpcProcessor& vmpcProcessorToUse)
        : AudioProcessorEditor(vmpcProcessorToUse), vmpcProcessor(vmpcProcessorToUse)
{
    setSize(320, 200);
}
