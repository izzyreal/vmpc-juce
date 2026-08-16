#include "gui/vector/View.hpp"

#include "gui/vector/GridWrapper.hpp"
#include "gui/vector/FlexBoxWrapper.hpp"
#include "gui/vector/ViewUtil.hpp"
#include "gui/vector/Constants.hpp"
#include "gui/focus/FocusHelper.hpp"
#include "gui/vector/TooltipOverlay.hpp"
#include "gui/vector/Menu.hpp"
#include "gui/vector/Disclaimer.hpp"
#include "gui/vector/About.hpp"
#include "gui/vector/Pad.hpp"
#include "gui/vector/PadTimer.hpp"
#include "gui/arrangement/ArrangementCatalog.hpp"
#include "gui/arrangement/ArrangementSelectorOverlay.hpp"
#include "gui/arrangement/ArrangementSurface.hpp"
#include "gui/ios/MobilePlatform.hpp"

#include "VmpcJuceResourceUtil.hpp"
#include "InitialWindowDimensions.hpp"
#include "performance/PerformanceManager.hpp"
#include "vf_freetype/vf_FreeTypeFaces.h"
#include "utils/ComponentUtils.hpp"

#include <Mpc.hpp>
#include <input/keyboard/KeyCodeHelper.hpp>
#include <input/HostInputEvent.hpp>
#include <controller/ClientEventController.hpp>

#include <raw_keyboard_input/raw_keyboard_input.h>

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <tuple>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

using namespace vmpc_juce::gui::vector;

namespace
{
    node makeFallbackRootNode()
    {
        node result;
        result.node_type = "flex_box";
        result.direction = "column";
        result.base_width = 1280;
        result.base_height = 720;
        return result;
    }
} // namespace

View::View(mpc::Mpc &mpcToUse,
           const std::function<void()> &showAudioSettingsDialog,
           const juce::AudioProcessor::WrapperType wrapperType,
           const std::function<bool()> &isInstrument,
           bool &shouldShowDisclaimer,
           const std::optional<std::string> &preferredArrangementId,
           std::function<void(const std::string &)> arrangementSelectedToUse,
           const bool menuExpanded,
           std::function<void(bool)> menuExpandedChangedToUse)
    : mpc(mpcToUse), getScale(
                         [this]
                         {
                             return static_cast<float>(getHeight()) /
                                    static_cast<float>(base_height);
                         }),

      getMainFontScaled(
          [&]() -> juce::Font &
          {
              mainFont.setHeight(Constants::BASE_FONT_SIZE * getScale());
              return mainFont;
          }),

      getMpc2000xlFaceplateGlyphsScaled(
          [&]() -> juce::Font &
          {
              mpc2000xlFaceplateGlyphsFont.setHeight(Constants::BASE_FONT_SIZE *
                                                     getScale());
              return mpc2000xlFaceplateGlyphsFont;
          }),

      getKeyTooltipFontScaled(
          [&]() -> juce::Font &
          {
              return keyTooltipFont;
          }),
      arrangementSelected(std::move(arrangementSelectedToUse)),
      menuExpandedChanged(std::move(menuExpandedChangedToUse)),
      processorWrapperType(wrapperType)
{
    phoneArrangementMode =
        gui::ios::isRunningOnIPhone() &&
        (wrapperType ==
             juce::AudioProcessor::WrapperType::wrapperType_Standalone ||
         wrapperType ==
             juce::AudioProcessor::WrapperType::wrapperType_AudioUnitv3);
    mainFontData =
        VmpcJuceResourceUtil::getResourceData("fonts/NeutralSans-Bold.ttf");
    FreeTypeFaces::addFaceFromMemory(1.f, 1.f, true, mainFontData.data(),
                                     static_cast<int>(mainFontData.size()));
    mainFont.setTypefaceName("Neutral Sans");
    mainFont = juce::Font(FreeTypeFaces::createTypefaceForFont(mainFont));

    mpc2000xlFaceplateGlyphsFontData = VmpcJuceResourceUtil::getResourceData(
        "fonts/mpc2000xl-faceplate-glyphs.ttf");
    FreeTypeFaces::addFaceFromMemory(
        1.f, 1.f, true, mpc2000xlFaceplateGlyphsFontData.data(),
        static_cast<int>(mpc2000xlFaceplateGlyphsFontData.size()), true);
    mpc2000xlFaceplateGlyphsFont.setTypefaceName("MPC2000XL Faceplate-Glyphs");
    mpc2000xlFaceplateGlyphsFont = juce::Font(
        FreeTypeFaces::createTypefaceForFont(mpc2000xlFaceplateGlyphsFont));

    keyTooltipFontData =
        VmpcJuceResourceUtil::getResourceData("fonts/FiraCode-SemiBold.ttf");
    FreeTypeFaces::addFaceFromMemory(
        1.f, 1.f, true, keyTooltipFontData.data(),
        static_cast<int>(keyTooltipFontData.size()));
    keyTooltipFont.setTypefaceName("Fira Code");
    keyTooltipFont.setTypefaceStyle("SemiBold");
    keyTooltipFont =
        juce::Font(FreeTypeFaces::createTypefaceForFont(keyTooltipFont));

    const bool shouldSynthesizeKeyRepeatsForSomeKeys =
        wrapperType ==
        juce::AudioProcessor::WrapperType::wrapperType_AudioUnitv3;

    keyboard = KeyboardFactory::instance(shouldSynthesizeKeyRepeatsForSomeKeys);

    focusHelper = new focus::FocusHelper(
        [clientEventController = mpc.clientEventController,
         focusHelperKeyboard = keyboard]
        {
            focusHelperKeyboard->allKeysUp();

            using FocusEvent = mpc::input::FocusEvent;
            const mpc::input::HostInputEvent hostInputEvent(
                FocusEvent{FocusEvent::Type::Lost});
            clientEventController->dispatchHostInput(hostInputEvent);
        });

    addAndMakeVisible(focusHelper);

    keyboard->hasFocus = [helper = focusHelper]
    {
        return helper->hasFocus();
    };

    const auto isVmpcKeyDown =
        [&](const std::initializer_list<mpc::input::keyboard::VmpcKeyCode>
                keyCodes)
    {
        for (auto &k : keyCodes)
        {
            if (keyboard->isKeyDown(mpc::input::keyboard::KeyCodeHelper::
                                        getPlatformFromVmpcKeyCode(k)))
            {
                return true;
            }
        }
        return false;
    };

    const auto getKeyboardMods =
        [&, isVmpcKeyDown]() -> std::tuple<bool, bool, bool>
    {
        using namespace mpc::input::keyboard;

        const bool shiftDown = isVmpcKeyDown(
            {VmpcKeyCode::VMPC_KEY_Shift, VmpcKeyCode::VMPC_KEY_LeftShift,
             VmpcKeyCode::VMPC_KEY_RightShift});

        const bool altDown =
            isVmpcKeyDown({VmpcKeyCode::VMPC_KEY_OptionOrAlt,
                           VmpcKeyCode::VMPC_KEY_LeftOptionOrAlt,
                           VmpcKeyCode::VMPC_KEY_RightOptionOrAlt});

        const bool ctrlDown = isVmpcKeyDown(
            {VmpcKeyCode::VMPC_KEY_Control, VmpcKeyCode::VMPC_KEY_LeftControl,
             VmpcKeyCode::VMPC_KEY_RightControl});

        return {shiftDown, altDown, ctrlDown};
    };

    keyboard->onKeyDownFn = [&, keyMods = getKeyboardMods](const int i)
    {
        if (!focusHelper->hasFocus() || about != nullptr)
        {
            return;
        }
        auto [shiftDown, altDown, ctrlDown] = keyMods();
        onKeyDown(i, ctrlDown, altDown, shiftDown);
    };

    keyboard->onKeyUpFn = [&, keyMods = getKeyboardMods](const int i)
    {
        if (!focusHelper->hasFocus() || about != nullptr)
        {
            return;
        }
        auto [shiftDown, altDown, ctrlDown] = keyMods();
        onKeyUp(i, ctrlDown, altDown, shiftDown);
    };

    setWantsKeyboardFocus(true);

    if (!phoneArrangementMode)
    {
        const auto jsonFileData = VmpcJuceResourceUtil::getResourceData(
            "json/" + layoutName + ".json");
        try
        {
            if (jsonFileData.empty())
            {
                throw std::runtime_error("layout resource is empty");
            }

            const nlohmann::json data = nlohmann::json::parse(jsonFileData);
            view_root = data.get<node>();
        }
        catch (const std::exception &e)
        {
            MLOG("Vector view failed to load layout '" + layoutName +
                 "': " + std::string(e.what()));
            view_root = makeFallbackRootNode();
        }
        catch (...)
        {
            MLOG("Vector view failed to load layout '" + layoutName +
                 "': unknown error");
            view_root = makeFallbackRootNode();
        }
        base_width = view_root.base_width;
        base_height = view_root.base_height;
    }
    else
    {
        base_width = 390;
        base_height = 844;
    }

    getScale = [this]
    {
        return static_cast<float>(getHeight()) /
               static_cast<float>(base_height);
    };

    tooltipOverlay = new TooltipOverlay();

    if (phoneArrangementMode)
    {
        const auto setupPath = std::string{"json/arrangements/default."} +
                               gui::arrangement::arrangementSetupFileExtension;
        const auto setupData = VmpcJuceResourceUtil::getResourceData(setupPath);
        std::string error;
        std::optional<gui::arrangement::ArrangementSetup> setup;
        if (!setupData.empty())
        {
            setup = gui::arrangement::deserializeArrangementSetup(
                std::string(setupData.begin(), setupData.end()), error);
        }
        if (!setup.has_value())
        {
            arrangementError = setupData.empty()
                                   ? "The bundled arrangement setup is missing."
                                   : error;
        }
        else if (const auto selected = gui::arrangement::resolveArrangementSlot(
                     *setup, preferredArrangementId))
        {
            arrangementSetup = *setup;
            activeArrangementSlot = *selected;
            arrangementSelected(
                arrangementSetup.slots[activeArrangementSlot]->id);
            const auto orientation =
                arrangementSetup.slots[activeArrangementSlot]->orientation;
            base_width = orientation == gui::arrangement::Orientation::portrait
                             ? 390
                             : 844;
            base_height = orientation == gui::arrangement::Orientation::portrait
                              ? 844
                              : 390;
            buildPhoneArrangement();
            if (wrapperType ==
                juce::AudioProcessor::WrapperType::wrapperType_Standalone)
            {
                gui::ios::setIPhoneOrientation(orientation);
            }
        }
        else
        {
            arrangementSetup = *setup;
            arrangementError =
                "The bundled arrangement setup has no occupied slots.";
        }
        if (!arrangementError.empty())
        {
            MLOG("iPhone arrangement setup: " + arrangementError);
        }
    }
    else
    {
        ViewUtil::createComponent(
            mpc, view_root, components, this, getScale, getMainFontScaled,
            getMpc2000xlFaceplateGlyphsScaled, getKeyTooltipFontScaled,
            mouseListeners, tooltipOverlay);
    }

    timerCallbackComponents =
        utils::findChildComponentsOfClass<WithSharedTimerCallback>(this);

    const auto openKeyboardScreen = [&]
    {
        mpc.getLayeredScreen()->openScreen("vmpc-keyboard");
    };
    const auto setKeyboardShortcutTooltipsVisibility =
        [&](const bool visibleEnabled)
    {
        tooltipOverlay->setAllKeyTooltipsVisibility(visibleEnabled);
    };

    closeAbout = [this]
    {
        if (about == nullptr)
        {
            return;
        }
        removeChildComponent(about);
        delete about;
        about = nullptr;
    };

    const auto openAbout = [this, wrapperType, isInstrument]
    {
        using W = juce::AudioProcessor::WrapperType;
        std::string wrapperTypeString;
        const std::string instOrFxString = isInstrument() ? " inst" : " fx";

        switch (wrapperType)
        {
            case W::wrapperType_VST:
                wrapperTypeString = "VST2" + instOrFxString;
                break;
            case W::wrapperType_VST3:
                wrapperTypeString = "VST3" + instOrFxString;
                break;
            case W::wrapperType_AudioUnit:
                wrapperTypeString = "AUv2" + instOrFxString;
                break;
            case W::wrapperType_AudioUnitv3:
                wrapperTypeString = "AUv3" + instOrFxString;
                break;
            case W::wrapperType_Standalone:
                wrapperTypeString = "Standalone";
                break;
            case W::wrapperType_LV2:
                wrapperTypeString = "LV2" + instOrFxString;
                break;
            case W::wrapperType_AAX:
                wrapperTypeString = "AAX" + instOrFxString;
                break;
            case W::wrapperType_Unity:
                wrapperTypeString = "Unity";
                break;
            case W::wrapperType_Undefined:
                wrapperTypeString = "Unknown";
        }

        if (about != nullptr)
        {
            removeChildComponent(about);
            delete about;
            about = nullptr;
        }

        about = new About(getScale, getMainFontScaled, closeAbout,
                          wrapperTypeString);
        addAndMakeVisible(about);
        resized();
    };

    initialRootWindowDimensions =
        InitialWindowDimensions::get(base_width, base_height);
    const auto resetWindowSize = [this]
    {
        getParentComponent()->setSize(initialRootWindowDimensions.first,
                                      initialRootWindowDimensions.second);
    };

    menu = new Menu(
#if TARGET_OS_IPHONE
        mpc,
#endif
        getScale, showAudioSettingsDialog, resetWindowSize, openKeyboardScreen,
        setKeyboardShortcutTooltipsVisibility, tooltipOverlay,
        getMainFontScaled, openAbout,
        phoneArrangementMode &&
                gui::arrangement::findFirstOccupiedSlot(arrangementSetup)
                    .has_value()
            ? std::function<void()>(
                  [this]
                  {
                      showArrangementSelector();
                  })
            : std::function<void()>(),
        std::function<void()>(),
        phoneArrangementMode,
        wrapperType,
        menuExpandedChanged);

    menu->setExpanded(menuExpanded);

    addAndMakeVisible(menu);
    addAndMakeVisible(tooltipOverlay);

    if (shouldShowDisclaimer)
    {
        const std::function deleteDisclaimerF = [this]
        {
            deleteDisclaimer();
        };
        disclaimer = new Disclaimer(getMainFontScaled, deleteDisclaimerF);
        addAndMakeVisible(disclaimer);
        shouldShowDisclaimer = false;
    }

    startTimer(WithSharedTimerCallback::baseIntervalMs);

    pads = utils::findChildComponentsOfClass<Pad>(this);

    static const std::map<uint8_t, uint8_t> padMap{
        {12, 0}, {13, 1}, {14, 2}, {15, 3}, {8, 4},  {9, 5},  {10, 6}, {11, 7},
        {4, 8},  {5, 9},  {6, 10}, {7, 11}, {0, 12}, {1, 13}, {2, 14}, {3, 15}};

    mpc.clientEventController->setActiveBankUiCallback =
        mpc::controller::SetActiveBankUiCallback(
            [&](const mpc::controller::Bank bank)
            {
                for (const auto &p : pads)
                {
                    p->registerBankSwitch(bank);
                }
            });

    mpc.getPerformanceManager().lock()->programPadEventUiCallback =
        mpc::performance::ProgramPadEventUiCallback(
            [&](const mpc::ProgramPadIndex programPadIndex,
                const mpc::VelocityOrPressure velocityOrPressure,
                const mpc::performance::UiCallbackPadEventType eventType)
            {
                if (pads.size() < 16)
                {
                    return;
                }
                const std::function isActiveBank = [&]
                {
                    const auto bank =
                        mpc::controller::programPadIndexToBank(programPadIndex);

                    const auto activeBank =
                        mpc.clientEventController->getActiveBank();

                    return bank == activeBank;
                };

                const auto pressType = isActiveBank()
                                           ? Pad::PressType::Secondary
                                           : Pad::PressType::Tertiary;

                const auto pad = pads[static_cast<size_t>(
                    padMap.at(static_cast<uint8_t>(programPadIndex % 16)))];

                if (eventType ==
                    mpc::performance::UiCallbackPadEventType::Press)
                {
                    pad->registerPress(pressType, programPadIndex,
                                       velocityOrPressure);
                }
                else if (eventType ==
                         mpc::performance::UiCallbackPadEventType::Aftertouch)
                {
                    pad->registerAftertouch(pressType, velocityOrPressure);
                }
                else if (eventType ==
                         mpc::performance::UiCallbackPadEventType::Release)
                {
                    pad->registerRelease(pressType);
                }
            });

    mpc.getPerformanceManager().lock()->physicalPadEventUiCallback =
        mpc::performance::PhysicalPadEventUiCallback(
            [&](const mpc::PhysicalPadIndex physicalPadIndex,
                const mpc::VelocityOrPressure velocityOrPressure,
                const mpc::performance::UiCallbackPadEventType eventType)
            {
                if (pads.size() < 16)
                {
                    return;
                }
                constexpr auto pressType = Pad::PressType::Primary;

                const auto pad = pads[static_cast<size_t>(
                    padMap.at(static_cast<uint8_t>(physicalPadIndex)))];

                if (eventType ==
                    mpc::performance::UiCallbackPadEventType::Press)
                {
                    pad->registerPress(pressType, physicalPadIndex,
                                       velocityOrPressure);
                }
                else if (eventType ==
                         mpc::performance::UiCallbackPadEventType::Aftertouch)
                {
                    pad->registerAftertouch(pressType, velocityOrPressure);
                }
                else if (eventType ==
                         mpc::performance::UiCallbackPadEventType::Release)
                {
                    pad->registerRelease(pressType);
                }
            });

    padTimer = new PadTimer(pads);
}

float View::getAspectRatio() const
{
    return static_cast<float>(base_width) / static_cast<float>(base_height);
}

std::pair<int, int> View::getInitialRootWindowDimensions()
{
    return initialRootWindowDimensions;
}

View::~View()
{
    stopTimer();

    delete padTimer;
    delete arrangementSelector;
    delete arrangementSurface;
    delete focusHelper;

    for (const auto &c : components)
    {
        delete c;
    }

    for (const auto &m : mouseListeners)
    {
        delete m;
    }

    delete tooltipOverlay;
    delete menu;
    delete disclaimer;
    delete about;
    delete keyboard;
    FreeTypeFaces::clearEverything();
}

void View::deleteDisclaimer()
{
    removeChildComponent(disclaimer);
    delete disclaimer;
    disclaimer = nullptr;
}

void View::resized()
{
    if (arrangementSurface != nullptr)
    {
        arrangementSurface->setBounds(getLocalBounds());
    }
    else if (!components.empty())
    {
        const auto rootComponent = components.front();
        assert(dynamic_cast<GridWrapper *>(rootComponent) != nullptr ||
               dynamic_cast<FlexBoxWrapper *>(rootComponent) != nullptr);
        rootComponent->setSize(getWidth(), getHeight());
    }
    if (tooltipOverlay != nullptr)
    {
        tooltipOverlay->setSize(getWidth(), getHeight());
    }

    const auto scale = getScale();
    const auto menuMargin = phoneArrangementMode ? 10.f : 2.f;
    auto menuScale = scale;
    if (phoneArrangementMode && menu != nullptr)
    {
        constexpr auto preferredIPhoneMenuScaleMultiplier = 3.3f;
        const auto availableWidth =
            std::max(1.f, static_cast<float>(getWidth()) -
                              (menuMargin * scale * 2.f));
        const auto fitScale =
            availableWidth / menu->getVisibleWidthAtScale1();
        menuScale =
            std::min(scale * preferredIPhoneMenuScaleMultiplier, fitScale);
    }
    if (menu != nullptr)
    {
        menu->setScaleMultiplier(scale > 0.f ? menuScale / scale : 1.f);
    }

    const auto menuWidth =
        (menu != nullptr ? menu->getRequiredWidthAtScale1()
                         : Menu::widthAtScale1) *
        menuScale;
    const auto menuHeight = (menu != nullptr
                                 ? menu->getRequiredHeightAtScale1()
                                 : Menu::heightAtScale1) *
                            menuScale;
    const auto menuX = static_cast<float>(getWidth()) - menuWidth -
                       (menuMargin * scale);
    const auto menuAtTop =
        phoneArrangementMode &&
        activeArrangementSlot < arrangementSetup.slots.size() &&
        arrangementSetup.slots[activeArrangementSlot].has_value() &&
        arrangementSetup.slots[activeArrangementSlot]->arrangement.menuAtTop;
    const auto menuY = menuAtTop
                           ? menuMargin * scale
                           : static_cast<float>(getHeight()) - menuHeight -
                                 (menuMargin * scale);

    if (menu != nullptr)
    {
        menu->setBounds(static_cast<int>(menuX), static_cast<int>(menuY),
                        static_cast<int>(menuWidth),
                        static_cast<int>(menuHeight));
    }

    const auto rect = getLocalBounds().reduced(
        static_cast<int>(static_cast<float>(getWidth()) * 0.25f),
        static_cast<int>(static_cast<float>(getHeight()) * 0.25f));

    if (disclaimer != nullptr)
    {
        if (phoneArrangementMode)
        {
            const auto widthFraction = getHeight() >= getWidth() ? 1.f : 0.8f;
            constexpr auto originalSizeFraction = 0.5f;
            disclaimer->setScaleMultiplier(widthFraction /
                                            originalSizeFraction);

            auto disclaimerBounds = getLocalBounds().withSizeKeepingCentre(
                juce::roundToInt(static_cast<float>(getWidth()) *
                                 widthFraction),
                juce::roundToInt(static_cast<float>(getHeight()) *
                                 widthFraction));
            disclaimer->setBounds(disclaimerBounds);
        }
        else
        {
            disclaimer->setScaleMultiplier(1.f);
            disclaimer->setBounds(rect);
        }
    }

    if (about != nullptr)
    {
        about->setBounds(rect);
    }
    if (arrangementSelector != nullptr)
    {
        arrangementSelector->setBounds(getLocalBounds());
        arrangementSelector->toFront(false);
    }
    repaint();
}

void View::paint(juce::Graphics &g)
{
    if (!phoneArrangementMode)
    {
        return;
    }
    g.fillAll(Constants::chassisColour);
    if (!arrangementError.empty())
    {
        g.setColour(juce::Colour(0xff343a38));
        const auto panel = getLocalBounds().reduced(
            std::max(18, juce::roundToInt(getWidth() * 0.08f)),
            std::max(18, juce::roundToInt(getHeight() * 0.2f)));
        g.fillRoundedRectangle(panel.toFloat(), 10.f);
        g.setColour(juce::Colour(0xffedf2ef));
        g.setFont(
            juce::Font(std::max(16.f, getHeight() * 0.025f), juce::Font::bold));
        g.drawFittedText("Arrangement setup unavailable\n\n" +
                             juce::String(arrangementError),
                         panel.reduced(20), juce::Justification::centred, 6);
    }
}

void View::onKeyDown(const int keyCode, const bool ctrlDown, const bool altDown,
                     const bool shiftDown) const
{
    using namespace mpc::input;
    const HostInputEvent hostInputEvent(
        KeyEvent{true, keyCode, shiftDown, ctrlDown, altDown});
    mpc.dispatchHostInput(hostInputEvent);
}

void View::onKeyUp(const int keyCode, const bool ctrlDown, const bool altDown,
                   const bool shiftDown) const
{
    using namespace mpc::input;
    const HostInputEvent hostInputEvent(
        KeyEvent{false, keyCode, shiftDown, ctrlDown, altDown});
    mpc.dispatchHostInput(hostInputEvent);
}

void View::timerCallback()
{
    for (const auto &c : timerCallbackComponents)
    {
        c->timerCallback();
    }
}

vmpc_juce::gui::focus::FocusHelper *View::getFocusHelper() const
{
    return focusHelper;
}

Keyboard *View::getKeyboard() const
{
    return keyboard;
}

bool View::usesPhoneArrangements() const
{
    return phoneArrangementMode;
}

void View::buildPhoneArrangement()
{
    arrangementError.clear();
    const auto &slot = arrangementSetup.slots[activeArrangementSlot];
    if (!slot.has_value() || slot->arrangement.nodes.empty())
    {
        arrangementError = "The selected arrangement slot is empty.";
        return;
    }

    const auto mainFontAtScale = [this](const float scale) -> juce::Font &
    {
        mainFont.setHeight(Constants::BASE_FONT_SIZE * scale);
        return mainFont;
    };
    const auto faceplateFontAtScale = [this](const float scale) -> juce::Font &
    {
        mpc2000xlFaceplateGlyphsFont.setHeight(Constants::BASE_FONT_SIZE *
                                               scale);
        return mpc2000xlFaceplateGlyphsFont;
    };
    const auto keyTooltipFontAtScale = [this](const float scale) -> juce::Font &
    {
        keyTooltipFont.setHeight(Constants::BASE_FONT_SIZE * scale);
        return keyTooltipFont;
    };

    std::string error;
    arrangementSurface = new gui::arrangement::ArrangementSurface(
        mpc, slot->arrangement, mainFontAtScale, faceplateFontAtScale,
        keyTooltipFontAtScale, tooltipOverlay, error);
    if (!error.empty())
    {
        delete arrangementSurface;
        arrangementSurface = nullptr;
        arrangementError = error;
        MLOG("iPhone arrangement: " + arrangementError);
        return;
    }
    addAndMakeVisible(arrangementSurface);
    arrangementSurface->toBack();
}

void View::refreshHardwareRegistrations()
{
    delete padTimer;
    padTimer = nullptr;
    pads = utils::findChildComponentsOfClass<Pad>(this);
    timerCallbackComponents =
        utils::findChildComponentsOfClass<WithSharedTimerCallback>(this);
    padTimer = new PadTimer(pads);
}

void View::showArrangementSelector()
{
    if (!phoneArrangementMode || arrangementSelector != nullptr)
    {
        return;
    }
    const auto mainFontAtScale = [this](const float scale) -> juce::Font &
    {
        mainFont.setHeight(Constants::BASE_FONT_SIZE * scale);
        return mainFont;
    };
    const auto faceplateFontAtScale = [this](const float scale) -> juce::Font &
    {
        mpc2000xlFaceplateGlyphsFont.setHeight(Constants::BASE_FONT_SIZE *
                                               scale);
        return mpc2000xlFaceplateGlyphsFont;
    };
    juce::Component::SafePointer<View> safeThis(this);
    arrangementSelector = new gui::arrangement::ArrangementSelectorOverlay(
        arrangementSetup, activeArrangementSlot, mainFontAtScale,
        faceplateFontAtScale,
        [safeThis](const std::size_t index)
        {
            juce::MessageManager::callAsync(
                [safeThis, index]
                {
                    if (safeThis != nullptr)
                    {
                        safeThis->selectArrangementSlot(index);
                    }
                });
        },
        [safeThis]
        {
            juce::MessageManager::callAsync(
                [safeThis]
                {
                    if (safeThis != nullptr)
                    {
                        safeThis->closeArrangementSelector();
                    }
                });
        });
    addAndMakeVisible(arrangementSelector);
    resized();
}

void View::closeArrangementSelector()
{
    if (arrangementSelector == nullptr)
    {
        return;
    }
    removeChildComponent(arrangementSelector);
    delete arrangementSelector;
    arrangementSelector = nullptr;
}

void View::selectArrangementSlot(const std::size_t index,
                                 const bool reportSelection)
{
    if (index >= arrangementSetup.slots.size() ||
        !arrangementSetup.slots[index].has_value() ||
        arrangementSetup.slots[index]->arrangement.nodes.empty())
    {
        return;
    }
    closeArrangementSelector();
    if (reportSelection)
    {
        arrangementSelected(arrangementSetup.slots[index]->id);
    }
    if (index == activeArrangementSlot)
    {
        return;
    }

    delete padTimer;
    padTimer = nullptr;
    pads.clear();
    timerCallbackComponents.clear();
    if (arrangementSurface != nullptr)
    {
        removeChildComponent(arrangementSurface);
        delete arrangementSurface;
        arrangementSurface = nullptr;
    }

    activeArrangementSlot = index;
    const auto orientation = arrangementSetup.slots[index]->orientation;
    base_width =
        orientation == gui::arrangement::Orientation::portrait ? 390 : 844;
    base_height =
        orientation == gui::arrangement::Orientation::portrait ? 844 : 390;
    buildPhoneArrangement();
    refreshHardwareRegistrations();
    if (processorWrapperType ==
        juce::AudioProcessor::WrapperType::wrapperType_Standalone)
    {
        gui::ios::setIPhoneOrientation(orientation);
    }
    initialRootWindowDimensions =
        InitialWindowDimensions::get(base_width, base_height);
    resized();
    if (auto *parent = getParentComponent())
    {
        parent->resized();
    }
}

void View::restoreArrangement(
    const std::optional<std::string> &arrangementId)
{
    if (!phoneArrangementMode)
    {
        return;
    }
    const auto resolved =
        gui::arrangement::resolveArrangementSlot(arrangementSetup,
                                                 arrangementId);
    if (!resolved.has_value())
    {
        return;
    }
    arrangementSelected(arrangementSetup.slots[*resolved]->id);
    selectArrangementSlot(*resolved, false);
}

void View::restoreMenuExpanded(const bool expanded)
{
    if (menu != nullptr)
    {
        menu->setExpanded(expanded);
    }
}

void View::toggleIPhoneFullscreen()
{
    iPhoneStatusBarHidden = !iPhoneStatusBarHidden;
    gui::ios::setIPhoneStatusBarHidden(iPhoneStatusBarHidden);
}
