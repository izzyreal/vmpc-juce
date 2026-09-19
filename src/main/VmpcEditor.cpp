#include "VmpcEditor.hpp"
#include "VmpcProcessor.hpp"

#include "gui/vector/View.hpp"
#include "gui/vector/Constants.hpp"
#include "gui/mobile/SafeArea.hpp"

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
                    },
                    [this](const juce::File &file)
                    {
                        return vmpcProcessor.startRecordingPreview(file);
                    },
                    [this]
                    {
                        vmpcProcessor.stopRecordingPreview();
                    },
                    [this]
                    {
                        return vmpcProcessor.isRecordingPreviewPlaying();
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

#if JUCE_IOS
    if (vmpcProcessor.wrapperType == juce::AudioProcessor::wrapperType_AudioUnitv3)
    {
        padDropBridge = std::make_unique<gui::ios::IosPadDropBridge>(*this);
    }
#endif

    startTimer(500);
}

VmpcEditor::~VmpcEditor()
{
#if JUCE_IOS
    padDropBridge.reset();
#endif
    vmpcProcessor.lastUIWidth = getWidth();
    vmpcProcessor.lastUIHeight = getHeight();
    setLookAndFeel(nullptr);
    delete view;
}

void VmpcEditor::timerCallback()
{
    if (initialFocusPending)
    {
        initialFocusPending = false;
        grabKeyboardFocus();
#if JUCE_IOS || JUCE_ANDROID
        startTimer(100);
#endif
    }
#if JUCE_IOS || JUCE_ANDROID
    // Safe insets can change without a component resize (e.g. rotating
    // between landscape orientations or showing system bars).
    if (getAvailableViewBounds() != availableViewBounds)
    {
        resized();
    }
#else
    stopTimer();
#endif
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

juce::Rectangle<int> VmpcEditor::getAvailableViewBounds() const
{
#if JUCE_IOS || JUCE_ANDROID
    if (getPeer() != nullptr)
    {
        const auto screenBounds = getScreenBounds();
        if (const auto *display =
                juce::Desktop::getInstance().getDisplays().getDisplayForRect(
                    screenBounds))
        {
            const auto safeBounds = gui::mobile::getSafeEditorScreenBounds(
                screenBounds, display->totalArea, display->safeAreaInsets);
            return getLocalArea(nullptr, safeBounds)
                .getIntersection(getLocalBounds());
        }
    }
#endif
    return getLocalBounds();
}

void VmpcEditor::moved()
{
#if JUCE_IOS || JUCE_ANDROID
    resized();
#endif
}

void VmpcEditor::parentHierarchyChanged()
{
#if JUCE_IOS
    if (padDropBridge != nullptr)
    {
        padDropBridge->refreshPeer();
    }
#endif
#if JUCE_IOS || JUCE_ANDROID
    resized();
#endif
}

void VmpcEditor::paint(juce::Graphics &g)
{
#if JUCE_IOS || JUCE_ANDROID
    g.fillAll(Constants::chassisColour);
#else
    juce::ignoreUnused(g);
#endif
}

void VmpcEditor::resized()
{
    if (view == nullptr)
    {
        return;
    }

    availableViewBounds = getAvailableViewBounds();
    if (availableViewBounds.isEmpty())
    {
        view->setBounds(availableViewBounds);
        return;
    }

    if (view->usesPhoneArrangements() &&
        vmpcProcessor.wrapperType ==
            juce::AudioProcessor::WrapperType::wrapperType_Standalone)
    {
        view->setBounds(availableViewBounds);
        return;
    }

    const float viewAspectRatio = view->getAspectRatio();
    const int parentWidth = availableViewBounds.getWidth();
    const int parentHeight = availableViewBounds.getHeight();

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

    view->setBounds(availableViewBounds.getX() + viewOffsetX,
                    availableViewBounds.getY() + viewOffsetY,
                    static_cast<int>(targetWidth),
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
