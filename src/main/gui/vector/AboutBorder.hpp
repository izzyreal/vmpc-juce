#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "TextWithLinks.hpp"
#include "VmpcJuceResourceUtil.hpp"

namespace vmpc_juce::gui::vector {

    class AboutBorder : public juce::Component {
        public:
            AboutBorder(const std::function<float()> &getScaleToUse) : getScale(getScaleToUse)
        {
            setInterceptsMouseClicks(false, false);
        }
            void paint(juce::Graphics &g)
            {
                const auto scale = getScale();
                const auto lineThickness = scale;
                const auto radius = scale * 3;
                const auto margin = scale * marginAtScale1;
                const auto rect = getLocalBounds().toFloat().reduced(lineThickness * 0.5).withTrimmedLeft(radius).withTrimmedRight(radius);
                const auto top = rect.withTrimmedBottom(getHeight() - margin);
                const auto bottom = rect.withTrimmedTop(getHeight() - margin);

                g.setColour(juce::Colours::white);
                g.fillRect(top); g.fillRect(bottom);

                const auto rect2 = getLocalBounds().toFloat().reduced(lineThickness * 0.5);

                g.setColour(juce::Colours::black);
                g.drawRoundedRectangle(rect2, radius, lineThickness);
            }

        private:
            const std::function<float()> &getScale;
            const float marginAtScale1 = 8.f;
    };
} // namespace vmpc_juce::gui::vector
