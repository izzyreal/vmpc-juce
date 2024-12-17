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

#include "SvgComponent.hpp"

namespace vmpc_juce::gui::vector {

    class Led : public SvgComponent {
        public:
            enum LedColor { RED, GREEN };

            Led(const std::string ledNameToUse, const LedColor ledColorToUse, const std::function<float()> &getScaleToUse)
                : SvgComponent({"led_off.svg", ledColorToUse == RED ? "led_on_red.svg" : "led_on_green.svg" }, nullptr, 0, getScaleToUse), ledColor(ledColorToUse), ledName(ledNameToUse)
        {
        }

            void setLedOnEnabled(const bool b)
            {
                if (ledOnEnabled == b) return;

                ledOnEnabled = b;

                if (!ledOnEnabled)
                {
                    setSvgPath("led_off.svg");
                    return;
                }

                if (ledColor == RED)
                {
                    setSvgPath("led_on_red.svg");
                    return;
                }

                setSvgPath("led_on_green.svg");
            }

            const std::string getLedName() { return ledName; }

        private:
            const LedColor ledColor;
            bool ledOnEnabled = false;
            const std::string ledName;

    };

} // namespace vmpc_juce::gui::vector
