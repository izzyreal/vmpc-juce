#include "View.hpp"

#include "GridWrapper.hpp"
#include "FlexBoxWrapper.hpp"
#include "ViewUtil.hpp"
#include "Led.hpp"
#include "TooltipOverlay.hpp"
#include "Menu.hpp"
#include "Disclaimer.hpp"
#include "About.hpp"
#include "Pad.hpp"
#include "Pot.hpp"
#include "Slider.hpp"

#include "VmpcJuceResourceUtil.hpp"
#include "InitialWindowDimensions.hpp"

#include "Mpc.hpp"
#include "input/KeyCodeHelper.hpp"

#include <raw_keyboard_input/raw_keyboard_input.h>
#include "gui/focus/FocusHelper.hpp"

#include <nlohmann/json.hpp>
#include "gui/vector/Constants.hpp"

#include "gui/vector/DataWheel.hpp"
#include "input/HostInputEvent.hpp"
#include "vf_freetype/vf_FreeTypeFaces.h"

#include <tuple>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

using namespace vmpc_juce::gui::vector;

template<class ComponentClass>
static std::vector<ComponentClass*> getChildComponentsOfClass(juce::Component *parent)
{
    std::vector<ComponentClass*> matches;

    for (int i = 0; i < parent->getNumChildComponents(); ++i)
    {
        auto *childComp = parent->getChildComponent(i);

        if (auto *c = dynamic_cast<ComponentClass*> (childComp))
            matches.push_back(c);

        auto childMatches = getChildComponentsOfClass<ComponentClass>(childComp);
        matches.insert(matches.end(), childMatches.begin(), childMatches.end());
    }

    return matches;
}

View::View(mpc::Mpc &mpcToUse,
           const std::function<void()> &showAudioSettingsDialog,
           const juce::AudioProcessor::WrapperType wrapperType,
           const std::function<bool()> isInstrument,
           bool &shouldShowDisclaimer)
    : mpc(mpcToUse),
    getScale([this] { return (float) getHeight() / (float) base_height; }),

    getMainFontScaled([&]() -> juce::Font& {
        mainFont.setHeight(Constants::BASE_FONT_SIZE * getScale());
        return mainFont;
    }),
    
    getMpc2000xlFaceplateGlyphsScaled([&]() -> juce::Font& {
        mpc2000xlFaceplateGlyphsFont.setHeight(Constants::BASE_FONT_SIZE * getScale());
        return mpc2000xlFaceplateGlyphsFont;
    }),

    getKeyTooltipFontScaled([&]() -> juce::Font& {
        return keyTooltipFont;
    })
{
    mainFontData = VmpcJuceResourceUtil::getResourceData("fonts/NeutralSans-Bold.ttf");
    FreeTypeFaces::addFaceFromMemory(1.f, 1.f, true, mainFontData.data(), static_cast<int>(mainFontData.size()));
    mainFont.setTypefaceName("Neutral Sans");
    mainFont = juce::Font(FreeTypeFaces::createTypefaceForFont(mainFont));

    mpc2000xlFaceplateGlyphsFontData = VmpcJuceResourceUtil::getResourceData("fonts/mpc2000xl-faceplate-glyphs.ttf");
    FreeTypeFaces::addFaceFromMemory(1.f, 1.f, true,
            mpc2000xlFaceplateGlyphsFontData.data(), static_cast<int>(mpc2000xlFaceplateGlyphsFontData.size()), true);
    mpc2000xlFaceplateGlyphsFont.setTypefaceName("MPC2000XL Faceplate-Glyphs");
    mpc2000xlFaceplateGlyphsFont = juce::Font(FreeTypeFaces::createTypefaceForFont(mpc2000xlFaceplateGlyphsFont));

    keyTooltipFontData = VmpcJuceResourceUtil::getResourceData("fonts/FiraCode-SemiBold.ttf");
    FreeTypeFaces::addFaceFromMemory(1.f, 1.f, true, keyTooltipFontData.data(), static_cast<int>(keyTooltipFontData.size()));
    keyTooltipFont.setTypefaceName("Fira Code");
    keyTooltipFont.setTypefaceStyle("SemiBold");
    keyTooltipFont = juce::Font(FreeTypeFaces::createTypefaceForFont(keyTooltipFont));

    const bool shouldSynthesizeKeyRepeatsForSomeKeys = wrapperType == juce::AudioProcessor::WrapperType::wrapperType_AudioUnitv3;

    keyboard = KeyboardFactory::instance(shouldSynthesizeKeyRepeatsForSomeKeys);

    focusHelper = new focus::FocusHelper([clientEventController = mpc.clientEventController, focusHelperKeyboard = keyboard]{

        focusHelperKeyboard->allKeysUp();

        using FocusEvent = mpc::input::FocusEvent;
        mpc::input::HostInputEvent hostInputEvent(FocusEvent{FocusEvent::Type::Lost});
        clientEventController->dispatchHostInput(hostInputEvent);
    });
    
    addAndMakeVisible(focusHelper);
    
    keyboard->hasFocus = [helper = focusHelper] { return helper->hasFocus(); };

    auto getKeyboardMods = [&]() -> std::tuple<bool, bool, bool> {
        using namespace mpc::input;

        bool shiftDown = keyboard->isKeyDown(KeyCodeHelper::getPlatformFromVmpcKeyCode(VmpcKeyCode::VMPC_KEY_Shift)) ||
            keyboard->isKeyDown(KeyCodeHelper::getPlatformFromVmpcKeyCode(VmpcKeyCode::VMPC_KEY_LeftShift)) ||
            keyboard->isKeyDown(KeyCodeHelper::getPlatformFromVmpcKeyCode(VmpcKeyCode::VMPC_KEY_RightShift));

        bool altDown = keyboard->isKeyDown(KeyCodeHelper::getPlatformFromVmpcKeyCode(VmpcKeyCode::VMPC_KEY_OptionOrAlt)) ||
            keyboard->isKeyDown(KeyCodeHelper::getPlatformFromVmpcKeyCode(VmpcKeyCode::VMPC_KEY_LeftOptionOrAlt)) ||
            keyboard->isKeyDown(KeyCodeHelper::getPlatformFromVmpcKeyCode(VmpcKeyCode::VMPC_KEY_RightOptionOrAlt));

        bool ctrlDown = keyboard->isKeyDown(KeyCodeHelper::getPlatformFromVmpcKeyCode(VmpcKeyCode::VMPC_KEY_Control)) ||
            keyboard->isKeyDown(KeyCodeHelper::getPlatformFromVmpcKeyCode(VmpcKeyCode::VMPC_KEY_LeftControl)) ||
            keyboard->isKeyDown(KeyCodeHelper::getPlatformFromVmpcKeyCode(VmpcKeyCode::VMPC_KEY_RightControl));

        return {shiftDown, altDown, ctrlDown };
    };
    
    keyboard->onKeyDownFn = [&, keyMods = getKeyboardMods](int i) {
        if (!focusHelper->hasFocus() || about != nullptr) return;
        auto [shiftDown, altDown, ctrlDown] = keyMods();
        onKeyDown(i, ctrlDown, altDown, shiftDown);
    };

    keyboard->onKeyUpFn = [&, keyMods = getKeyboardMods](int i) {
        if (!focusHelper->hasFocus() || about != nullptr) return;
        auto [shiftDown, altDown, ctrlDown] = keyMods();
        onKeyUp(i, ctrlDown, altDown, shiftDown);
    };

    setWantsKeyboardFocus(true);

    const auto jsonFileData = VmpcJuceResourceUtil::getResourceData("json/" + name + ".json");
    nlohmann::json data = nlohmann::json::parse(jsonFileData);

    view_root = data.template get<node>();

    base_width = view_root.base_width;
    base_height = view_root.base_height;

    getScale = [this] { return (float) getHeight() / (float) base_height; };

    tooltipOverlay = new TooltipOverlay();

    ViewUtil::createComponent(mpc, view_root, components, this, getScale, getMainFontScaled, getMpc2000xlFaceplateGlyphsScaled, getKeyTooltipFontScaled, mouseListeners, tooltipOverlay);

    pads = getChildComponentsOfClass<Pad>(this);
    leds = getChildComponentsOfClass<Led>(this);
    dataWheel = getChildComponentsOfClass<DataWheel>(this).front();
    sliderCap = getChildComponentsOfClass<SliderCap>(this).front();

    for (auto &pot : getChildComponentsOfClass<Pot>(this))
    {
        if (pot->potType == Pot::PotType::MAIN_VOLUME) volPot = pot;
        else if (pot->potType == Pot::PotType::REC_GAIN) recPot = pot;
    }

    const auto openKeyboardScreen = [&] { mpc.getLayeredScreen()->openScreen("vmpc-keyboard"); };
    const auto setKeyboardShortcutTooltipsVisibility = [&](const bool visibleEnabled){
        tooltipOverlay->setAllKeyTooltipsVisibility(visibleEnabled);
    };

    const auto closeAbout = [this] {
        if (about == nullptr) return;
        removeChildComponent(about);
        delete about;
        about = nullptr;
    };

    const auto openAbout = [this, closeAbout, wrapperType, isInstrument] {
        
        using W = juce::AudioProcessor::WrapperType;
        std::string wrapperTypeString;
        const std::string instOrFxString = isInstrument() ? " inst" : " fx";

        switch (wrapperType)
        {
            case W::wrapperType_VST: wrapperTypeString = "VST2" + instOrFxString; break;
            case W::wrapperType_VST3: wrapperTypeString = "VST3" + instOrFxString; break;
            case W::wrapperType_AudioUnit: wrapperTypeString = "AUv2" + instOrFxString; break;
            case W::wrapperType_AudioUnitv3: wrapperTypeString = "AUv3" + instOrFxString; break;
            case W::wrapperType_Standalone: wrapperTypeString = "Standalone"; break;
            case W::wrapperType_LV2: wrapperTypeString = "LV2" + instOrFxString; break;
            case W::wrapperType_AAX: wrapperTypeString = "AAX" + instOrFxString; break;
            case W::wrapperType_Unity: wrapperTypeString = "Unity"; break;
            case W::wrapperType_Undefined: wrapperTypeString = "Unknown";
        }

        if (about != nullptr)
        {
            removeChildComponent(about);
            delete about;
            about = nullptr;
        }

        about = new About(getScale, getMainFontScaled, closeAbout, wrapperTypeString);
        addAndMakeVisible(about);
        resized();
    };

    initialRootWindowDimensions = InitialWindowDimensions::get(base_width, base_height);
    const auto resetWindowSize = [this]{
        getParentComponent()->setSize(initialRootWindowDimensions.first, initialRootWindowDimensions.second);
    };

    menu = new Menu(
#if TARGET_OS_IPHONE
            mpc,
#endif
            getScale,
            showAudioSettingsDialog,
            resetWindowSize,
            openKeyboardScreen,
            setKeyboardShortcutTooltipsVisibility,
            tooltipOverlay,
            getMainFontScaled,
            openAbout,
            wrapperType);

    addAndMakeVisible(menu);
    addAndMakeVisible(tooltipOverlay);

    if (shouldShowDisclaimer)
    {
        const std::function<void()> deleteDisclaimerF = [this] { deleteDisclaimer(); };
        disclaimer = new Disclaimer(getMainFontScaled, deleteDisclaimerF);
        addAndMakeVisible(disclaimer);
        shouldShowDisclaimer = false;
    }

    startTimer(10);
}

const float View::getAspectRatio()
{
    return (float) base_width / base_height;
}

const std::pair<int, int> View::getInitialRootWindowDimensions()
{
    return initialRootWindowDimensions;
}

View::~View()
{
    delete focusHelper;
    
    for (auto &c : components)
    {
        delete c;
    }

    for (auto &m : mouseListeners)
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

    auto rootComponent = components.front();

    assert(dynamic_cast<GridWrapper*>(rootComponent) != nullptr ||
            dynamic_cast<FlexBoxWrapper*>(rootComponent) != nullptr);

    rootComponent->setSize(getWidth(), getHeight());
    tooltipOverlay->setSize(getWidth(), getHeight());

    const auto scale = getScale();
    const auto menuMargin = 2.f;
    const auto menuWidth = Menu::widthAtScale1 * scale;
    const auto menuHeight = Menu::heightAtScale1 * scale;
    const auto menuWidthWithMargin = (Menu::widthAtScale1 + menuMargin) * scale;
    const auto menuHeightWithMargin = (Menu::heightAtScale1 + menuMargin) * scale;
    const auto menuX = static_cast<float>(getWidth()) - menuWidthWithMargin;
    const auto menuY = static_cast<float>(getHeight()) - menuHeightWithMargin;
    
    menu->setBounds(static_cast<int>(menuX), static_cast<int>(menuY), static_cast<int>(menuWidth), static_cast<int>(menuHeight));

    auto rect = getLocalBounds().reduced(static_cast<int>(static_cast<float>(getWidth()) * 0.25f), static_cast<int>(static_cast<float>(getHeight()) * 0.25f));

    if (disclaimer != nullptr)
    {
        disclaimer->setBounds(rect);
    }

    if (about != nullptr)
    {
        about->setBounds(rect);
    }
}

void View::onKeyDown(int keyCode, bool ctrlDown, bool altDown, bool shiftDown)
{
    using namespace mpc::input;
    HostInputEvent hostInputEvent(KeyEvent { true, keyCode, shiftDown, ctrlDown, altDown});
    mpc.dispatchHostInput(hostInputEvent);
}

void View::onKeyUp(int keyCode, bool ctrlDown, bool altDown, bool shiftDown)
{
    using namespace mpc::input;
    HostInputEvent hostInputEvent(KeyEvent { false, keyCode, shiftDown, ctrlDown, altDown});
    mpc.dispatchHostInput(hostInputEvent);
}

void View::timerCallback()
{
    for (auto &p : pads)
    {
        p->sharedTimerCallback();
    }

    for (auto &l : leds)
    {
        l->sharedTimerCallback();
    }

    dataWheel->sharedTimerCallback();
    recPot->sharedTimerCallback();
    volPot->sharedTimerCallback();
    sliderCap->sharedTimerCallback();
}
