#include "VmpcAuxLcdLookAndFeel.hpp"

using namespace vmpc_juce::gui;

void VmpcAuxLcdLookAndFeel::drawLcdPixel(juce::Graphics& g, int x, int y)
{
    g.fillRect(juce::Rectangle<int>(x * LCD_PIXEL_SIZE, y * LCD_PIXEL_SIZE, LCD_PIXEL_SIZE, LCD_PIXEL_SIZE));
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
