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
#include "Constants.hpp"

using namespace vmpc_juce::gui::bitmap;

juce::Rectangle<int>& Constants::dataWheelRect()
{
    static auto rect = juce::Rectangle<int>(378, 415, 171, 171);
	return rect;
}

juce::Rectangle<int>& Constants::lcdRect()
{
    static auto rect = juce::Rectangle<int>(116, 102, 496, 120);
    return rect;
}

const juce::Colour Constants::LCD_ON = juce::Colour::fromRGB(86, 61, 145);
const juce::Colour Constants::LCD_OFF = juce::Colour::fromRGB(170, 248, 218);
const juce::Colour Constants::LCD_HALF_ON = juce::Colour::fromRGB(128, 154, 181);

juce::Rectangle<int>& Constants::recKnobRect()
{
    static auto rect = juce::Rectangle<int>(1014, 183, 72, 73);
	return rect;
}

juce::Rectangle<int>& Constants::volKnobRect()
{
    static auto rect = juce::Rectangle<int>(1137, 181, 74, 75);
	return rect;
}

juce::Rectangle<int>& Constants::sliderRect()
{
    static auto rect = juce::Rectangle<int>(33, 668, 128, 247);
	return rect;
}
