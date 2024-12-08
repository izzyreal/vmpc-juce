#define ENABLE_GUI_INSPECTOR 0

#include "VmpcEditor.hpp"
#include "VmpcProcessor.hpp"

#include "gui/vector/Constants.hpp"

#include "VmpcJuceResourceUtil.hpp"

#if ENABLE_GUI_INSPECTOR == 1
#include <melatonin_inspector/melatonin_inspector.h>
#endif

using namespace vmpc_juce;
using namespace vmpc_juce::gui::vector;

VmpcEditor::VmpcEditor(VmpcProcessor& vmpcProcessorToUse)
        : AudioProcessorEditor(vmpcProcessorToUse), vmpcProcessor(vmpcProcessorToUse)
{
    setSize(320, 200);
}

VmpcEditor::~VmpcEditor()
{
#if ENABLE_GUI_INSPECTOR == 1
    delete inspector;
#endif
}

void VmpcEditor::resized()
{
}

