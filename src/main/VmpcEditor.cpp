#define ENABLE_GUI_INSPECTOR 0

#include "VmpcEditor.hpp"
#include "VmpcProcessor.hpp"

#include "gui/vector/View.hpp"

#if ENABLE_GUI_INSPECTOR == 1
#include <melatonin_inspector/melatonin_inspector.h>
#endif

using namespace vmpc_juce;
using namespace vmpc_juce::gui::vector;

VmpcEditor::VmpcEditor(VmpcProcessor& vmpcProcessorToUse)
    : AudioProcessorEditor(vmpcProcessorToUse), vmpcProcessor(vmpcProcessorToUse)
{

    setWantsKeyboardFocus(true);

    view = new View(vmpcProcessor.mpc, vmpcProcessor.showAudioSettingsDialog, vmpcProcessor.wrapperType);

    const auto viewAspectRatio = view->getAspectRatio();

    auto initialWindowWidth = vmpcProcessor.lastUIWidth;
    auto initialWindowHeight = vmpcProcessor.lastUIHeight;

    const auto initialDimensions = view->getInitialRootWindowDimensions();

    if (initialWindowWidth == 0 || initialWindowHeight == 0 /* || check if aspect ratio is different */) 
    {
        initialWindowWidth = initialDimensions.first;
        initialWindowHeight = initialDimensions.second;
    }

#if JUCE_IOS
    if (juce::JUCEApplication::isStandaloneApp())
    {
        const auto primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();

        if (primaryDisplay != nullptr)
        {
            const auto area = primaryDisplay->userArea;
            setSize(area.getWidth(), area.getHeight());
        }
        else
        {
            setSize(initialWindowWidth, initialWindowHeight);
        }
    }
    else
    {
        setSize(initialWindowWidth, initialWindowHeight);
    }
#else

    setSize(initialWindowWidth, initialWindowHeight);
    const bool useCornerResizer = vmpcProcessor.wrapperType != juce::AudioProcessor::wrapperType_AudioUnitv3;
    setResizable(true, useCornerResizer);
    getConstrainer()->setFixedAspectRatio(viewAspectRatio);
    getConstrainer()->setSizeLimits(initialDimensions.first / 2, initialDimensions.second / 2, initialDimensions.first * 1.1f, initialDimensions.second * 1.1f);
    setLookAndFeel(&lookAndFeel);

#endif

    addAndMakeVisible(view);

    startTimer(100);

#if ENABLE_GUI_INSPECTOR == 1
    inspector = new melatonin::Inspector(*this);
    inspector->setVisible(true);
    inspector->toggle(true);
#endif
}

VmpcEditor::~VmpcEditor()
{
    vmpcProcessor.lastUIWidth = getWidth();
    vmpcProcessor.lastUIHeight = getHeight();
    setLookAndFeel(nullptr);
    delete view;
#if ENABLE_GUI_INSPECTOR == 1
    delete inspector;
#endif
}

void VmpcEditor::timerCallback()
{
    this->grabKeyboardFocus();
    stopTimer();
}

void VmpcEditor::resized()
{
    view->setBounds(0, 0, getWidth(), getHeight());
}

