#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector {

    class TooltipOverlay : public juce::Component {
        public:
            TooltipOverlay()
            {
                setWantsKeyboardFocus(false);
                setInterceptsMouseClicks(false, false);
            }

    };

} // namespace vmpc_juce::gui::vector
