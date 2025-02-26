#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//#include "KeyTooltip.hpp"

namespace vmpc_juce::gui::vector {

    class TooltipOverlay : public juce::Component {
        public:
            TooltipOverlay()
            {
                setWantsKeyboardFocus(false);
                setInterceptsMouseClicks(false, false);
            }

            void setAllKeyTooltipsVisibility(const bool visibleEnabled)
            {
                for (auto &c : getChildren())
                {
                    //if (dynamic_cast<KeyTooltip*>(c) == nullptr)
                    {
                        //continue;
                    }
                    c->setVisible(visibleEnabled);
                }
            }

            void setKeyTooltipVisibility(const std::string label, const bool visibleEnabled)
            {
                for (auto &c : getChildren())
                {
                    //if (auto t = dynamic_cast<KeyTooltip*>(c); t != nullptr)
                    {
                        //if (t->getHardwareLabel() == label)
                        {
                            //t->setVisible(visibleEnabled);
                        }
                    }
                }
            }
    };

} // namespace vmpc_juce::gui::vector
