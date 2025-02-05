#include "juce_gui_basics/juce_gui_basics.h"
#define DEBUG_NODES 0

#include "View.hpp"

#include "GridWrapper.hpp"
#include "FlexBoxWrapper.hpp"
#include "ViewUtil.hpp"
#include "Led.hpp"
#include "LedController.hpp"
#include "TooltipOverlay.hpp"
#include "Menu.hpp"
#include "Disclaimer.hpp"
#include "About.hpp"

#include "VmpcJuceResourceUtil.hpp"
#include "InitialWindowDimensions.hpp"

#include "Mpc.hpp"
#include "controls/Controls.hpp"
#include "controls/KeyEvent.hpp"
#include "controls/KeyEventHandler.hpp"

#include <raw_keyboard_input/raw_keyboard_input.h>

#include <nlohmann/json.hpp>
#include "gui/vector/Constants.hpp"

#include "vf_freetype/vf_FreeTypeFaces.h"

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

View::View(mpc::Mpc &mpcToUse, const std::function<void()> &showAudioSettingsDialog, juce::AudioProcessor::WrapperType wrapperType)
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
    FreeTypeFaces::addFaceFromMemory(1.f, 1.f, true, mainFontData.data(), mainFontData.size());
    mainFont.setTypefaceName("Neutral Sans");
    mainFont = juce::Font(FreeTypeFaces::createTypefaceForFont(mainFont));

    mpc2000xlFaceplateGlyphsFontData = VmpcJuceResourceUtil::getResourceData("fonts/mpc2000xl-faceplate-glyphs.ttf");
    FreeTypeFaces::addFaceFromMemory(1.f, 1.f, true,
            mpc2000xlFaceplateGlyphsFontData.data(), mpc2000xlFaceplateGlyphsFontData.size(), true);
    mpc2000xlFaceplateGlyphsFont.setTypefaceName("MPC2000XL Faceplate-Glyphs");
    mpc2000xlFaceplateGlyphsFont = juce::Font(FreeTypeFaces::createTypefaceForFont(mpc2000xlFaceplateGlyphsFont));

    keyTooltipFontData = VmpcJuceResourceUtil::getResourceData("fonts/FiraCode-SemiBold.ttf");
    FreeTypeFaces::addFaceFromMemory(1.f, 1.f, true, keyTooltipFontData.data(), keyTooltipFontData.size());
    keyTooltipFont.setTypefaceName("Fira Code");
    keyTooltipFont.setTypefaceStyle("SemiBold");
    keyTooltipFont = juce::Font(FreeTypeFaces::createTypefaceForFont(keyTooltipFont));

    keyboard = KeyboardFactory::instance(this);

    keyboard->onKeyDownFn = [&](int i) { onKeyDown(i); };
    keyboard->onKeyUpFn = [&](int i) { onKeyUp(i); };

    setWantsKeyboardFocus(true);

    const auto jsonFileData = VmpcJuceResourceUtil::getResourceData("json/" + name + ".json");
    nlohmann::json data = nlohmann::json::parse(jsonFileData);

    view_root = data.template get<node>();

    base_width = view_root.base_width;
    base_height = view_root.base_height;

    getScale = [this] { return (float) getHeight() / (float) base_height; };

    tooltipOverlay = new TooltipOverlay();

    ViewUtil::createComponent(mpc, view_root, components, this, getScale, getMainFontScaled, getMpc2000xlFaceplateGlyphsScaled, getKeyTooltipFontScaled, mouseListeners, tooltipOverlay);

    Led *fullLevelLed = nullptr;
    Led *sixteenLevelsLed = nullptr;
    Led *nextSeqLed = nullptr;
    Led *trackMuteLed = nullptr;
    Led *padBankALed = nullptr;
    Led *padBankBLed = nullptr;
    Led *padBankCLed = nullptr;
    Led *padBankDLed = nullptr;
    Led *afterLed = nullptr;
    Led *undoSeqLed = nullptr;
    Led *recLed = nullptr;
    Led *overDubLed = nullptr;
    Led *playLed = nullptr;

    const auto leds = getChildComponentsOfClass<Led>(this);

    for (auto &l : leds)
    {
        const auto ledName = l->getLedName();
        if (ledName == "full_level_led") fullLevelLed = l;
        else if (ledName == "16_levels_led") sixteenLevelsLed = l;
        else if (ledName == "next_seq_led") nextSeqLed = l;
        else if (ledName == "track_mute_led") trackMuteLed = l;
        else if (ledName == "bank_a_led") padBankALed = l;
        else if (ledName == "bank_b_led") padBankBLed = l;
        else if (ledName == "bank_c_led") padBankCLed = l;
        else if (ledName == "bank_d_led") padBankDLed = l;
        else if (ledName == "after_led") afterLed = l;
        else if (ledName == "undo_seq_led") undoSeqLed = l;
        else if (ledName == "rec_led") recLed = l;
        else if (ledName == "over_dub_led") overDubLed = l;
        else if (ledName == "play_led") playLed = l;
    }

    if (leds.size() == 13)
    {
        ledController = new LedController(mpc, fullLevelLed, sixteenLevelsLed, nextSeqLed, trackMuteLed, padBankALed, padBankBLed, padBankCLed, padBankDLed, afterLed, undoSeqLed, recLed, overDubLed, playLed);
        
        ledController->setPadBankA(true);
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
        keyboard->onKeyDownFn = [&](int i) { onKeyDown(i); };
        keyboard->onKeyUpFn = [&](int i) { onKeyUp(i); };
    };

    std::string wrapperTypeString;

    switch (wrapperType)
    {
        case juce::AudioProcessor::WrapperType::wrapperType_VST: wrapperTypeString = "VST2"; break;
        case juce::AudioProcessor::WrapperType::wrapperType_VST3: wrapperTypeString = "VST3"; break;
        case juce::AudioProcessor::WrapperType::wrapperType_AudioUnit: wrapperTypeString = "AUv2"; break;
        case juce::AudioProcessor::WrapperType::wrapperType_AudioUnitv3: wrapperTypeString = "AUv3"; break;
        case juce::AudioProcessor::WrapperType::wrapperType_Standalone: wrapperTypeString = "Standalone"; break;
        case juce::AudioProcessor::WrapperType::wrapperType_LV2: wrapperTypeString = "LV2"; break;
        case juce::AudioProcessor::WrapperType::wrapperType_AAX: wrapperTypeString = "AAX"; break;
        case juce::AudioProcessor::WrapperType::wrapperType_Unity: wrapperTypeString = "Unity"; break;
        case juce::AudioProcessor::WrapperType::wrapperType_Undefined: wrapperTypeString = "Unknown";
    }

    const auto openAbout = [this, closeAbout, wrapperTypeString] {
        if (about != nullptr)
        {
            removeChildComponent(about);
            delete about;
            about = nullptr;
        }

        about = new About(getScale, getMainFontScaled, closeAbout, wrapperTypeString);
        keyboard->onKeyUpFn = {};
        keyboard->onKeyDownFn = {};
        addAndMakeVisible(about);
        resized();
    };

    initialRootWindowDimensions = InitialWindowDimensions::get(base_width, base_height);
    const auto resetWindowSize = [this]{
        getParentComponent()->setSize(initialRootWindowDimensions.first, initialRootWindowDimensions.second);
    };

    menu = new Menu(mpc, getScale, showAudioSettingsDialog, resetWindowSize, openKeyboardScreen, setKeyboardShortcutTooltipsVisibility, tooltipOverlay, getMainFontScaled, openAbout, wrapperType);

    addAndMakeVisible(menu);
    addAndMakeVisible(tooltipOverlay);

    const std::function<void()> deleteDisclaimerF = [this] { deleteDisclaimer(); };
    disclaimer = new Disclaimer(getMainFontScaled, deleteDisclaimerF);
    //addAndMakeVisible(disclaimer);
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
    for (auto &c : components)
    {
        delete c;
    }

    for (auto &m : mouseListeners)
    {
        delete m;
    }

    delete ledController;
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
    const auto menuX = getWidth() - menuWidthWithMargin;
    const auto menuY = getHeight() - menuHeightWithMargin;
    
    menu->setBounds(menuX, menuY, menuWidth, menuHeight);

    auto rect = getLocalBounds().reduced(getWidth() * 0.25, getHeight() * 0.25);

    if (disclaimer != nullptr)
    {
        disclaimer->setBounds(rect);
    }

    if (about != nullptr)
    {
        about->setBounds(rect);
    }
}

void View::onKeyDown(int keyCode) {
    mpc.getControls()->getKeyEventHandler().lock()->handle(mpc::controls::KeyEvent(keyCode, true));
}

void View::onKeyUp(int keyCode) {
    mpc.getControls()->getKeyEventHandler().lock()->handle(mpc::controls::KeyEvent(keyCode, false));
}

