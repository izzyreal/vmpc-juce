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

#include <tuple>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

using namespace vmpc_juce::gui::vector;

View::View(mpc::Mpc &mpcToUse,
           const std::function<void()> &showAudioSettingsDialog,
           const juce::AudioProcessor::WrapperType wrapperType,
           const std::function<bool()> &isInstrument,
           bool &shouldShowDisclaimer)
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
          })
{
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

    const auto jsonFileData =
        VmpcJuceResourceUtil::getResourceData("json/" + layoutName + ".json");
    const nlohmann::json data = nlohmann::json::parse(jsonFileData);

    view_root = data.get<node>();

    base_width = view_root.base_width;
    base_height = view_root.base_height;

    getScale = [this]
    {
        return static_cast<float>(getHeight()) /
               static_cast<float>(base_height);
    };

    tooltipOverlay = new TooltipOverlay();

    ViewUtil::createComponent(
        mpc, view_root, components, this, getScale, getMainFontScaled,
        getMpc2000xlFaceplateGlyphsScaled, getKeyTooltipFontScaled,
        mouseListeners, tooltipOverlay);

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
        getMainFontScaled, openAbout, wrapperType);

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
    if (components.empty())
    {
        return;
    }

    const auto rootComponent = components.front();

    assert(dynamic_cast<GridWrapper *>(rootComponent) != nullptr ||
           dynamic_cast<FlexBoxWrapper *>(rootComponent) != nullptr);

    rootComponent->setSize(getWidth(), getHeight());
    tooltipOverlay->setSize(getWidth(), getHeight());

    const auto scale = getScale();
    constexpr auto menuMargin = 2.f;
    const auto menuWidth = Menu::widthAtScale1 * scale;
    const auto menuHeight = Menu::heightAtScale1 * scale;
    const auto menuWidthWithMargin = (Menu::widthAtScale1 + menuMargin) * scale;
    const auto menuHeightWithMargin =
        (Menu::heightAtScale1 + menuMargin) * scale;
    const auto menuX = static_cast<float>(getWidth()) - menuWidthWithMargin;
    const auto menuY = static_cast<float>(getHeight()) - menuHeightWithMargin;

    menu->setBounds(static_cast<int>(menuX), static_cast<int>(menuY),
                    static_cast<int>(menuWidth), static_cast<int>(menuHeight));

    const auto rect = getLocalBounds().reduced(
        static_cast<int>(static_cast<float>(getWidth()) * 0.25f),
        static_cast<int>(static_cast<float>(getHeight()) * 0.25f));

    if (disclaimer != nullptr)
    {
        disclaimer->setBounds(rect);
    }

    if (about != nullptr)
    {
        about->setBounds(rect);
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
