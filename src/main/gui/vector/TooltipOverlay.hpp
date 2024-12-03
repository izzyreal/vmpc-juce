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
            
            void paint(juce::Graphics &g) override
            {
                //g.fillAll(juce::Colours::white.withAlpha(0.2f));
            }
    };

} // namespace vmpc_juce::gui::vector
