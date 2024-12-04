#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Tooltip.hpp"

namespace vmpc_juce::gui::vector {

    class TooltipOverlay : public juce::Component {
        public:
            TooltipOverlay()
            {
                setWantsKeyboardFocus(false);
                setInterceptsMouseClicks(false, false);
            }

            void setAllTooltipsVisibility(const bool visibleEnabled)
            {
                for (auto &c : getChildren())
                {
                    if (dynamic_cast<Tooltip*>(c) == nullptr)
                    {
                        continue;
                    }
                    c->setVisible(visibleEnabled);
                }
            }

            void setTooltipVisibility(const std::string label, const bool visibleEnabled)
            {
                for (auto &c : getChildren())
                {
                    if (auto t = dynamic_cast<Tooltip*>(c); t != nullptr)
                    {
                        if (t->getHardwareLabel() == label)
                        {
                            t->setVisible(visibleEnabled);
                        }
                    }
                }
            }
    };

} // namespace vmpc_juce::gui::vector
