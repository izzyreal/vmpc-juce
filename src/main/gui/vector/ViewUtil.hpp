#pragma once

#include "Node.hpp"
#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>
#include <functional>

namespace mpc { class Mpc; }

namespace vmpc_juce::gui::vector {

class ViewUtil
{
    public:
        static float getLabelHeight(const std::string &text,
                const std::function<float()> &getScale);

        static void createComponents(
                mpc::Mpc &mpc,
                node &n,
                std::vector<juce::Component*> &components,
                juce::Component* parent,
                const std::function<float()> &getScale,
                const std::function<juce::Font&()> &getMainFontScaled,
                const std::function<juce::Font&()> &getMpc2000xlFaceplateGlyphsScaled,
                std::vector<juce::MouseListener*> &mouseListeners,
                juce::Component *tooltipOverlay);

        static void createComponent(
                mpc::Mpc &mpc,
                node &n,
                std::vector<juce::Component*> &components,
                juce::Component *parent,
                const std::function<float()> &getScale,
                const std::function<juce::Font&()> &getMainFontScaled,
                const std::function<juce::Font&()> &getMpc2000xlFaceplateGlyphsScaled,
                std::vector<juce::MouseListener*> &mouseListeners,
                juce::Component *tooltipOverlay);

        static juce::Point<int> getShadowDimensions(const float shadowSize, const float scale)
        {
            static const float base_size = 8.f;
            static const float base_size_x = base_size;
            static const float base_size_y = base_size_x * 0.7f;
            const float magic_factor = shadowSize * scale * 0.25f;
            const auto size_x = base_size_x * magic_factor;
            const auto size_y = base_size_y * magic_factor;
            juce::Point<float> dimensions = { size_x, size_y };
            return dimensions.roundToInt();
        }
};
} // namespace vmpc_juce:gui::vector
