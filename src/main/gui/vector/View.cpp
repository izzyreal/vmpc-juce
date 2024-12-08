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

#include "VmpcJuceResourceUtil.hpp"
#include "Mpc.hpp"
#include "controls/Controls.hpp"
#include "controls/KeyEvent.hpp"
#include "controls/KeyEventHandler.hpp"

#include <raw_keyboard_input/raw_keyboard_input.h>

#include <nlohmann/json.hpp>

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

View::View(
        mpc::Mpc &mpc,
        const std::function<float()> &getScaleToUse,
        const std::function<juce::Font&()> &getNimbusSansScaledToUse,
        const std::function<void()> &showAudioSettingsDialog,
        const std::function<void()> &resetWindowSize)
    : getScale(getScaleToUse), getNimbusSansScaled(getNimbusSansScaledToUse)
{
    keyboard = KeyboardFactory::instance(this);

    keyboard->onKeyDownFn = [&](int keyCode) {
        mpc.getControls()->getKeyEventHandler().lock()->handle(mpc::controls::KeyEvent(keyCode, true));
    };

    keyboard->onKeyUpFn = [&](int keyCode) {
        mpc.getControls()->getKeyEventHandler().lock()->handle(mpc::controls::KeyEvent(keyCode, false));
    };

    setWantsKeyboardFocus(true);

    const auto jsonFileData = VmpcJuceResourceUtil::getResourceData("json/" + name + ".json");
    nlohmann::json data = nlohmann::json::parse(jsonFileData);

    view_root = data.template get<node>();

    tooltipOverlay = new TooltipOverlay();

    ViewUtil::createComponent(mpc, view_root, components, this, getScale, getNimbusSansScaled, mouseListeners, tooltipOverlay);

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
        const auto name = l->getLedName();
        if (name == "full_level_led") fullLevelLed = l;
        else if (name == "16_levels_led") sixteenLevelsLed = l;
        else if (name == "next_seq_led") nextSeqLed = l;
        else if (name == "track_mute_led") trackMuteLed = l;
        else if (name == "bank_a_led") padBankALed = l;
        else if (name == "bank_b_led") padBankBLed = l;
        else if (name == "bank_c_led") padBankCLed = l;
        else if (name == "bank_d_led") padBankDLed = l;
        else if (name == "after_led") afterLed = l;
        else if (name == "undo_seq_led") undoSeqLed = l;
        else if (name == "rec_led") recLed = l;
        else if (name == "over_dub_led") overDubLed = l;
        else if (name == "play_led") playLed = l;
    }

    ledController = new LedController(mpc, fullLevelLed, sixteenLevelsLed, nextSeqLed, trackMuteLed, padBankALed, padBankBLed, padBankCLed, padBankDLed, afterLed, undoSeqLed, recLed, overDubLed, playLed);

    ledController->setPadBankA(true);

    const auto openKeyboardScreen = [&mpc] { mpc.getLayeredScreen()->openScreen("vmpc-keyboard"); };
    const auto setKeyboardShortcutTooltipsVisibility = [&](const bool visibleEnabled){
        tooltipOverlay->setAllKeyTooltipsVisibility(visibleEnabled);
    };

    menu = new Menu(getScale, showAudioSettingsDialog, resetWindowSize, openKeyboardScreen, setKeyboardShortcutTooltipsVisibility, tooltipOverlay, getNimbusSansScaled, mpc);

    addAndMakeVisible(menu);
    addAndMakeVisible(tooltipOverlay);

    const std::function<void()> deleteDisclaimerF = [this] { deleteDisclaimer(); };
    disclaimer = new Disclaimer(getNimbusSansScaled, deleteDisclaimerF);
    addAndMakeVisible(disclaimer);
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
}

void View::deleteDisclaimer()
{
    removeChildComponent(disclaimer);
    delete disclaimer;
    disclaimer = nullptr;
}

void View::resized()
{
    assert(!components.empty());

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
}

