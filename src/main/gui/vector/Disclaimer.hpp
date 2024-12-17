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

#include "juce_graphics/juce_graphics.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <melatonin_blur/melatonin_blur.h>

namespace vmpc_juce::gui::vector {

    class Disclaimer : public juce::Component, juce::Timer {
        public:
            Disclaimer(const std::function<juce::Font&()> &getNimbusSansScaledToUse, const std::function<void()> &deleteMeToUse)
                : getNimbusSansScaled(getNimbusSansScaledToUse), deleteMe(deleteMeToUse)
            {
                startTimer(10000);
            }

            void mouseDown(const juce::MouseEvent &) override
            {
                deleteMe();
            }

            void timerCallback() override
            {
                deleteMe();
            }

            void paint(juce::Graphics &g) override
            {
                const std::string text = "MPC® and Akai Professional® are registered trademarks of\n" 
                                         "inMusic Brands. Inc. This emulator is not affiliated with\n"  
                                         "inMusic and use of the MPC® and Akai Professional® names has\n"
                                         "not been authorized, sponsored or otherwise approved by inMusic.";

                auto font = getNimbusSansScaled();
                font.setHeight(font.getHeight() * 1.5);
                g.setFont(font);

                auto rect = getLocalBounds()/*.reduced(10)*/;
                rect = rect.withTrimmedTop(((getHeight() - (font.getHeight() * 4)) / 2) - font.getHeight());
                rect = rect.withTrimmedBottom(((getHeight() - (font.getHeight() * 4)) / 2) - font.getHeight());
                rect.reduce(2.f, 2.f);

                juce::Path p;
                p.addRoundedRectangle(rect, 5);

                melatonin::DropShadow shadow;
                shadow.setColor(juce::Colours::black.withAlpha(0.5f));
                shadow.setOffset(5, 5);
                shadow.setRadius(8);
                shadow.render(g, p);

                g.setColour(juce::Colours::white);
                g.fillRoundedRectangle(rect.toFloat(), 5);

                g.setColour(juce::Colours::black);
                g.drawRoundedRectangle(rect.toFloat(), 5, 2);

                g.drawFittedText(text, getLocalBounds().reduced(font.getHeight()), juce::Justification::centred, 4);
            }

        private:
            const std::function<juce::Font&()> &getNimbusSansScaled;
            const std::function<void()> deleteMe;
    };
} // namespace vmpc_juce::gui::vector
