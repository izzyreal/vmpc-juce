#include "TooltipOverlay.hpp"

#include "KeyTooltip.hpp"

using namespace vmpc_juce::gui::vector;

TooltipOverlay::TooltipOverlay()
{
    setWantsKeyboardFocus(false);
    setInterceptsMouseClicks(false, false);
}

void TooltipOverlay::setAllKeyTooltipsVisibility(const bool visibleEnabled)
{
    for (auto &c : getChildren())
    {
        if (dynamic_cast<KeyTooltip *>(c) == nullptr)
        {
            continue;
        }
        c->setVisible(visibleEnabled);
    }
}

void TooltipOverlay::setKeyTooltipVisibility(const std::string label,
                                             const bool visibleEnabled)
{
    for (auto &c : getChildren())
    {
        if (auto t = dynamic_cast<KeyTooltip *>(c); t != nullptr)
        {
            if (t->getHardwareLabel() == label)
            {
                t->setVisible(visibleEnabled);
            }
        }
    }
}
