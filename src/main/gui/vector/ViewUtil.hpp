/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
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
                const std::function<juce::Font&()> &getNimbusSansScaled,
                const std::function<juce::Font&()> &getMpc2000xlFaceplateGlyphsScaled,
                std::vector<juce::MouseListener*> &mouseListeners,
                juce::Component *tooltipOverlay);

        static void createComponent(
                mpc::Mpc &mpc,
                node &n,
                std::vector<juce::Component*> &components,
                juce::Component *parent,
                const std::function<float()> &getScale,
                const std::function<juce::Font&()> &getNimbusSansScaled,
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
