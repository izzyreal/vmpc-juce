#include "VmpcLookAndFeel.h"
#include "Constants.h"

using namespace juce;

void VmpcLookAndFeel::drawCornerResizer (Graphics& g, int, int, bool, bool)
{
    const int blockSize = 3;
    const int rows = 7;
    for (int i = 0; i < rows; i++)
    {
        const int y = i * blockSize;

        for (int j = 0; j < i; j++)
        {
            const int x = ((rows - j) - 2) * blockSize;
            g.setColour(Constants::LCD_HALF_ON);
            g.fillRect(juce::Rectangle<int>(x, y, blockSize, blockSize));
            g.setColour(Constants::LCD_ON);
            g.fillRect(juce::Rectangle<int>(x, y, blockSize - 1, blockSize - 1));
        }
    }
}