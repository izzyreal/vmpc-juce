#pragma once

#include "gui/vector/Node.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

namespace vmpc_juce::gui::arrangement
{
    class PreviewViewUtil
    {
    public:
        static void createComponent(
            gui::vector::node &node, std::vector<juce::Component *> &components,
            juce::Component *parent, const std::function<float()> &getScale,
            const std::function<juce::Font &()> &getMainFontScaled,
            const std::function<juce::Font &()>
                &getMpc2000xlFaceplateGlyphsScaled);

    private:
        static void createComponents(
            gui::vector::node &node, std::vector<juce::Component *> &components,
            juce::Component *parent, const std::function<float()> &getScale,
            const std::function<juce::Font &()> &getMainFontScaled,
            const std::function<juce::Font &()>
                &getMpc2000xlFaceplateGlyphsScaled);
    };
} // namespace vmpc_juce::gui::arrangement
