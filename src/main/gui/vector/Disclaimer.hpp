#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <melatonin_blur/melatonin_blur.h>

namespace vmpc_juce::gui::vector {

    class Disclaimer : public juce::Component, juce::Timer {
        public:
            Disclaimer(const std::function<juce::Font&()> &getMainFontScaledToUse, const std::function<void()> &deleteMeToUse)
                : getMainFontScaled(getMainFontScaledToUse), deleteMe(deleteMeToUse)
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

                auto font = getMainFontScaled();
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
            const std::function<juce::Font&()> &getMainFontScaled;
            const std::function<void()> deleteMe;
    };
} // namespace vmpc_juce::gui::vector
