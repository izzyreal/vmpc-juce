#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector
{

    class TooltipOverlay : public juce::Component
    {
    public:
        TooltipOverlay();

        void setAllKeyTooltipsVisibility(const bool visibleEnabled);

        void setKeyTooltipVisibility(const std::string label,
                                     const bool visibleEnabled);
    };

} // namespace vmpc_juce::gui::vector
