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
#include "VmpcAuxLcdLookAndFeel.hpp"

#include "gui/bitmap/Constants.hpp"

using namespace vmpc_juce::gui;
using namespace vmpc_juce::gui::bitmap;

void VmpcAuxLcdLookAndFeel::drawLcdPixel(juce::Graphics& g, int x, int y)
{
    g.setColour(Constants::LCD_HALF_ON);
    g.fillRect(juce::Rectangle<int>(x * LCD_PIXEL_SIZE, y * LCD_PIXEL_SIZE, LCD_PIXEL_SIZE, LCD_PIXEL_SIZE));
    g.setColour(Constants::LCD_ON);
    g.fillRect(juce::Rectangle<int>(x * LCD_PIXEL_SIZE, y * LCD_PIXEL_SIZE, LCD_PIXEL_SIZE - 1, LCD_PIXEL_SIZE - 1));
}

void VmpcAuxLcdLookAndFeel::drawCornerResizer(juce::Graphics& g, int w, int h, bool, bool)
{
    const int rows = 5;
    const int xOffset = (w / VmpcAuxLcdLookAndFeel::LCD_PIXEL_SIZE) - rows;
    const int yOffset = (h / VmpcAuxLcdLookAndFeel::LCD_PIXEL_SIZE) - rows;

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < i; j++)
        {
            const int x = (rows - j);
            drawLcdPixel(g, x + xOffset - 1, i + yOffset - 1);
        }
    }
}
