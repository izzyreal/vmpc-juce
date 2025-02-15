#include "VmpcEditor.hpp"
#include "VmpcProcessor.hpp"

#include "gui/vector/View.hpp"

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
}

VmpcEditor::~VmpcEditor()
{
    vmpcProcessor.lastUIWidth = getWidth();
    vmpcProcessor.lastUIHeight = getHeight();
    setLookAndFeel(nullptr);
    delete view;
}

void VmpcEditor::timerCallback()
{
    this->grabKeyboardFocus();
    stopTimer();
}

void VmpcEditor::resized()
{
    const float viewAspectRatio = view->getAspectRatio();
    const int parentWidth = getWidth();
    const int parentHeight = getHeight();
    
    float targetWidth = parentWidth;
    float targetHeight = targetWidth / viewAspectRatio;
    
    if (targetHeight > parentHeight) {
        targetHeight = parentHeight;
        targetWidth = targetHeight * viewAspectRatio;
    }
    
    const int viewOffsetX = (parentWidth - targetWidth) / 2;
    const int viewOffsetY = (parentHeight - targetHeight) / 2;
    
    view->setBounds(viewOffsetX, viewOffsetY, targetWidth, targetHeight);
}
