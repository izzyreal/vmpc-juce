#include "KbEditor.h"

#include "Constants.h"

#include <Mpc.hpp>
#include <lcdgui/kbeditor/KbEditor.hpp>

#include <vector>

using namespace std;

KbEditor::KbEditor(mpc::Mpc& mpc)
	: VmpcComponent("keyboard-editor"), mpc(mpc)
{
	lcd = Image(Image::RGB, 800, 600, true);
}

void KbEditor::checkDirty()
{
    auto kbEditorLcd = mpc.getKbEditor().lock();
    vector<vector<bool>> pixels(400, vector<bool>(400));
    
//	if (kbEditorLcd->IsDirty())
//	{
        kbEditorLcd->Draw(&pixels);
        Colour c;
    
        auto halfOn = Constants::LCD_HALF_ON;
        auto on = Constants::LCD_ON;
        auto off = Constants::LCD_OFF;
    
        const auto rectX = 0;
        const auto rectY = 0;
        const auto rectRight = 400;
        const auto rectBottom = 400;
    
        for (int x = rectX; x < rectRight; x++)
        {
            for (int y = rectY; y < rectBottom; y++)
            {
                const auto x_x2 = x * 2;
                const auto y_x2 = y * 2;
    
                if ((pixels)[x][y])
                {
                    c = halfOn;
                    lcd.setPixelAt(x_x2, y_x2, on);
                }
                else {
                    c = off;
                    lcd.setPixelAt(x_x2, y_x2, c);
                }
    
                lcd.setPixelAt(x_x2 + 1, y_x2, c);
                lcd.setPixelAt(x_x2 + 1, y_x2 + 1, c);
                lcd.setPixelAt(x_x2, y_x2 + 1, c);
            }
//        }
        
        repaint();
	}
}

void KbEditor::timerCallback()
{
	checkDirty();
}

void KbEditor::paint(Graphics& g)
{
	g.drawImageAt(lcd, 0, 0);
}
