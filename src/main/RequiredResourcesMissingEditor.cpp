#include "RequiredResourcesMissingEditor.hpp"

#include "VmpcProcessor.hpp"

using namespace vmpc_juce;

RequiredResourcesMissingEditor::RequiredResourcesMissingEditor(
    VmpcProcessor &processor, const std::string &message)
    : AudioProcessorEditor(processor)
{
    addAndMakeVisible(text);
    text.setMultiLine(true);
    text.setReadOnly(true);
    text.setScrollbarsShown(true);
    text.setText(message, juce::dontSendNotification);
    setSize(900, 600);
}

void RequiredResourcesMissingEditor::resized()
{
    text.setBounds(getLocalBounds().reduced(16));
}
