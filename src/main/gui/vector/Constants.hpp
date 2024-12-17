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

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector {

    class Constants {
        public:
            static juce::Colour chassisColour;
            static juce::Colour labelColour;
            static juce::Colour darkLabelColour;
            static juce::Colour greyFacePaintColour;
            static juce::Colour betweenChassisAndLabelColour;

            static juce::Colour lcdOn;
            static juce::Colour lcdOnLight;
            static juce::Colour lcdOff;
            static juce::Colour lcdOffBacklit;

            static float BASE_FONT_SIZE;
            static float LINE_SIZE;
            static float lineThickness1;
            static float lineThickness2;
    };

} // namespace vmpc_juce::gui::vector
