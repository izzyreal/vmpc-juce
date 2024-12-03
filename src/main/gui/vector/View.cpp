#define DEBUG_NODES 0

#include "View.hpp"

#include "GridWrapper.hpp"
#include "FlexBoxWrapper.hpp"
#include "ViewUtil.hpp"
#include "Led.hpp"
#include "LedController.hpp"

#include "VmpcJuceResourceUtil.hpp"
#include "Mpc.hpp"
#include "controls/Controls.hpp"
#include "controls/KeyEvent.hpp"
#include "controls/KeyEventHandler.hpp"

#include <raw_keyboard_input/raw_keyboard_input.h>

#include <nlohmann/json.hpp>

using namespace vmpc_juce::gui::vector;

using json = nlohmann::json;

namespace vmpc_juce::gui::vector {

    static void from_json(const json& j, node& n)
    {
        if (j.contains("include"))
        {
            const auto jsonFileData = VmpcJuceResourceUtil::getResourceData("json/" + j.at("include").get<std::string>() + ".json");

            json data = json::parse(jsonFileData);
            n = data.template get<node>();

            if (j.contains("area")) j.at("area").get_to(n.area);

            if (j.contains("margin"))
            {
                if (j.at("margin").is_array())
                {
                    j.at("margin").get_to(n.margins);
                    n.margin = 0.f;
                }
                else
                {
                    j.at("margin").get_to(n.margin);
                }
            }
            else
            {
                n.margin = 0.f;
            }
        }
        else
        {
            if (j.contains("margin"))
            {
                if (j.at("margin").is_array())
                {
                    j.at("margin").get_to(n.margins);
                    n.margin = 0.f;
                }
                else
                {
                    j.at("margin").get_to(n.margin);
                }
            }
            else
            {
                n.margin = 0.f;
            }

            if (j.contains("name"))             j.at("name").get_to(n.name);
            if (j.contains("type"))             j.at("type").get_to(n.node_type);
            if (j.contains("svg"))              j.at("svg").get_to(n.svg);
            if (j.contains("children"))         j.at("children").get_to(n.children);
            if (j.contains("label"))            j.at("label").get_to(n.label);
            if (j.contains("label_style"))      j.at("label_style").get_to(n.label_style);
            if (j.contains("direction"))        j.at("direction").get_to(n.direction);
            if (j.contains("flex_grow"))        j.at("flex_grow").get_to(n.flex_grow); else n.flex_grow = 0.f;
            if (j.contains("align_items"))      j.at("align_items").get_to(n.align_items);
            if (j.contains("row_fractions"))    j.at("row_fractions").get_to(n.row_fractions);
            if (j.contains("column_fractions")) j.at("column_fractions").get_to(n.column_fractions);
            if (j.contains("area"))             j.at("area").get_to(n.area);
            if (j.contains("justify_items"))    j.at("justify_items").get_to(n.justify_items);
            if (j.contains("width"))            j.at("width").get_to(n.width);
            if (j.contains("hide_svg"))         j.at("hide_svg").get_to(n.hide_svg); else n.hide_svg = false;
            if (j.contains("shadow_darkness"))  j.at("shadow_darkness").get_to(n.shadow_darkness); else n.shadow_darkness = 0.f;
            if (j.contains("is_inner_shadow"))  j.at("is_inner_shadow").get_to(n.is_inner_shadow); else n.is_inner_shadow = false;
            if (j.contains("magic_multiplier")) j.at("magic_multiplier").get_to(n.magic_multiplier); else n.magic_multiplier = 0.f;
            if (j.contains("hardware_label"))   j.at("hardware_label").get_to(n.hardware_label);

            if (j.contains("shadow_size"))
            {
                j.at("shadow_size").get_to(n.shadow_size);

                if (n.shadow_size > 0.f)
                {
                    for (auto &c : n.children)
                    {
                        c.shadow_size = n.shadow_size;
                    }
                }
            }
            else
            {
                n.shadow_size = 0.f;
            }

            if (j.contains("label_text_to_calculate_width"))
            {
                j.at("label_text_to_calculate_width").get_to(n.label_text_to_calculate_width);
            }
            else if (j.contains("label"))
            {
                j.at("label").get_to(n.label_text_to_calculate_width);
            }
        }


        if (!n.area.empty())
        {
            // JUCE's withArea(rowStart, columnStart, rowEnd, columnEnd) API expects exclusive
            // rowEnd and columnEnd arguments. I find that unintuitive, so in the layouts I
            // specify inclusive end arguments, and do a fix-up here.
            n.area[2] += 1;
            n.area[3] += 1;
        }

#if DEBUG_NODES == 1
        printf("=== Deserialized node ===\n");
        if (!n.name.empty())        printf("-        name: %s\n", n.name.c_str());
        if (!n.svg.empty())         printf("-         svg: %s\n", n.svg.c_str());
        if (!n.children.empty())    printf("-    children: %zu\n", n.children.size());
        if (n.margin != 0.f)        printf("-      margin: %f\n", n.margin);
        if (!n.label.empty())       printf("-       label: %s\n", n.label.c_str());
        if (!n.label_style.empty()) printf("- label_style: %s\n", n.label_style.c_str());
        if (n.flex_grow != 0.f)     printf("-   flex_grow: %f\n", n.flex_grow);

        if (!n.row_fractions.empty())
        {
            printf("- row_fractions:");
            for (auto& f : n.row_fractions)
                printf(" %i ", f);
            printf("\n");
        }

        if (!n.area.empty())
        {
            printf("-          area:");
            for (auto& f : n.area)
                printf(" %i ", f);
            printf("\n");
        }

        if (!n.margins.empty())
        {
            printf("-       margins:");
            for (auto& f : n.margins)
                printf(" %f ", f);
            printf("\n");
        }

        printf("=========================\n");
#endif
    }
} // namespace vmpc_juce::gui::vector

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

View::View(mpc::Mpc &mpc, const std::function<float()> &getScaleToUse, const std::function<juce::Font&()> &getNimbusSansScaledToUse)
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
    json data = json::parse(jsonFileData);

    view_root = data.template get<node>();

    ViewUtil::createComponent(mpc, view_root, components, this, getScale, getNimbusSansScaled, mouseListeners);

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
}

void View::resized()
{
    assert(!components.empty());

    auto rootComponent = components.front();

    assert(dynamic_cast<GridWrapper*>(rootComponent) != nullptr ||
            dynamic_cast<FlexBoxWrapper*>(rootComponent) != nullptr);

    rootComponent->setSize(getWidth(), getHeight());
}

