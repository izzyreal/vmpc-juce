#include "VmpcEditor.hpp"
#include "VmpcProcessor.hpp"

#include "gui/vector/View.hpp"

#include <raw_keyboard_input/raw_keyboard_input.h>

using namespace vmpc_juce;
using namespace vmpc_juce::gui::vector;

VmpcEditor::VmpcEditor(VmpcProcessor &vmpcProcessorToUse)
    : AudioProcessorEditor(vmpcProcessorToUse),
      vmpcProcessor(vmpcProcessorToUse)
{
    setWantsKeyboardFocus(true);

    std::function<bool()> isInstrument = [&]
    {
        const std::string auComponentType = vmpcProcessor.auComponentType();
        return auComponentType.empty() || auComponentType == "aumu";
    };

    view = new View(vmpcProcessor.mpc, vmpcProcessor.showAudioSettingsDialog,
                    vmpcProcessor.wrapperType, isInstrument,
                    vmpcProcessor.shouldShowDisclaimer,
                    vmpcProcessor.getActiveArrangementId(),
                    [this](const std::string &arrangementId)
                    {
                        vmpcProcessor.setActiveArrangementId(arrangementId);
                    },
                    vmpcProcessor.isMenuExpanded(),
                    [this](const bool expanded)
                    {
                        vmpcProcessor.setMenuExpanded(expanded);
                    });

    auto initialWindowWidth = vmpcProcessor.lastUIWidth;
    auto initialWindowHeight = vmpcProcessor.lastUIHeight;

    const auto initialDimensions = view->getInitialRootWindowDimensions();

    if (initialWindowWidth == 0 ||
        initialWindowHeight == 0 /* || check if aspect ratio is different */)
    {
        initialWindowWidth = initialDimensions.first;
        initialWindowHeight = initialDimensions.second;
    }

#if JUCE_IOS || JUCE_ANDROID
    if (juce::JUCEApplication::isStandaloneApp())
    {
        const auto primaryDisplay =
            juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();

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

    const auto viewAspectRatio = view->getAspectRatio();

    if (juce::PluginHostType::getHostPath().containsIgnoreCase("ardour"))
    {
        setSize(
            static_cast<int>(static_cast<float>(initialWindowWidth) * 0.5f),
            static_cast<int>(static_cast<float>(initialWindowHeight) * 0.5f));
        constexpr bool useCornerResizer = true;
        setResizable(true, useCornerResizer);
    }
    else
    {
        setSize(initialWindowWidth, initialWindowHeight);
        const bool useCornerResizer =
            vmpcProcessor.wrapperType !=
            juce::AudioProcessor::wrapperType_AudioUnitv3;
        setResizable(true, useCornerResizer);
    }

    const auto hostPath = juce::PluginHostType::getHostPath();

#if __linux__
    const bool shouldFixAspectRatio = !hostPath.containsIgnoreCase("ardour") &&
                                      !hostPath.containsIgnoreCase("carla");
#else
    const bool shouldFixAspectRatio = !hostPath.containsIgnoreCase("ardour");
#endif

    if (shouldFixAspectRatio)
    {
        getConstrainer()->setFixedAspectRatio(viewAspectRatio);
    }

    getConstrainer()->setSizeLimits(
        static_cast<int>(static_cast<float>(initialDimensions.first) / 8.f),
        static_cast<int>(static_cast<float>(initialDimensions.second) / 8.f),
        static_cast<int>(static_cast<float>(initialDimensions.first) * 1.1f),
        static_cast<int>(static_cast<float>(initialDimensions.second) * 1.1f));

    setLookAndFeel(&lookAndFeel);

#endif

    addAndMakeVisible(view);

    startTimer(500);
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
    grabKeyboardFocus();
    stopTimer();
}

void VmpcEditor::restoreActiveArrangement(
    const std::optional<std::string> &arrangementId)
{
    if (view != nullptr)
    {
        view->restoreArrangement(arrangementId);
    }
}

void VmpcEditor::restoreMenuExpanded(const bool expanded)
{
    if (view != nullptr)
    {
        view->restoreMenuExpanded(expanded);
    }
}

void VmpcEditor::resized()
{
    if (view->usesPhoneArrangements() &&
        vmpcProcessor.wrapperType ==
            juce::AudioProcessor::WrapperType::wrapperType_Standalone)
    {
        const auto landscape = getWidth() > getHeight();
        if (stablePhoneStandaloneViewBounds.isEmpty() ||
            landscape != stablePhoneLandscape)
        {
            stablePhoneLandscape = landscape;
            stablePhoneStandaloneViewBounds = getLocalBounds();
        }
        const auto width =
            std::min(getWidth(), stablePhoneStandaloneViewBounds.getWidth());
        const auto height =
            std::min(getHeight(), stablePhoneStandaloneViewBounds.getHeight());
        view->setBounds((getWidth() - width) / 2, (getHeight() - height) / 2,
                        width, height);
        return;
    }

    const float viewAspectRatio = view->getAspectRatio();
    const int parentWidth = getWidth();
    const int parentHeight = getHeight();

    float targetWidth = static_cast<float>(parentWidth);
    float targetHeight = targetWidth / viewAspectRatio;

    if (targetHeight > static_cast<float>(parentHeight))
    {
        targetHeight = static_cast<float>(parentHeight);
        targetWidth = targetHeight * viewAspectRatio;
    }

    const int viewOffsetX =
        static_cast<int>((static_cast<float>(parentWidth) - targetWidth) / 2.f);
    const int viewOffsetY = static_cast<int>(
        (static_cast<float>(parentHeight) - targetHeight) / 2.f);

    view->setBounds(viewOffsetX, viewOffsetY, static_cast<int>(targetWidth),
                    static_cast<int>(targetHeight));
}

void VmpcEditor::handleRawKeyEvent(const juce::RawKeyEvent &k)
{
    const auto hostType = juce::PluginHostType();
    const auto hostPath = juce::PluginHostType::getHostPath();

    if (!hostType.isRenoise() && !hostPath.containsIgnoreCase("ardour"))
    {
        return;
    }

    Keyboard::processKeyEvent(k.keyCode, k.keyDown);
}
