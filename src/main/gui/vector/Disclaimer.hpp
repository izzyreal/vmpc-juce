#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <melatonin_blur/melatonin_blur.h>

namespace vmpc_juce::gui::vector
{

    class Disclaimer : public juce::Component, juce::Timer
    {
    public:
        Disclaimer(const std::function<juce::Font &()> &getMainFontScaledToUse,
                   const std::function<void()> &deleteMeToUse)
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

        void setScaleMultiplier(const float multiplier)
        {
            scaleMultiplier = multiplier;
            repaint();
        }

        void paint(juce::Graphics &g) override
        {
            const std::string text =
                "MPC® and Akai Professional® are registered trademarks of\n"
                "inMusic Brands. Inc. This emulator is not affiliated with\n"
                "inMusic and use of the MPC® and Akai Professional® names has\n"
                "not been authorized, sponsored or otherwise approved by "
                "inMusic.";

            auto font = getMainFontScaled();
            font.setHeight(font.getHeight() * 1.5f * scaleMultiplier);
            g.setFont(font);

            auto rect = getLocalBounds() /*.reduced(10)*/;
            rect = rect.withTrimmedTop(static_cast<int>(
                ((static_cast<float>(getHeight()) - (font.getHeight() * 4)) /
                 2) -
                font.getHeight()));
            rect = rect.withTrimmedBottom(static_cast<int>(
                ((static_cast<float>(getHeight()) - (font.getHeight() * 4)) /
                 2) -
                font.getHeight()));
            const auto edgeInset =
                juce::roundToInt(2.f * scaleMultiplier);
            rect.reduce(edgeInset, edgeInset);

            juce::Path p;
            p.addRoundedRectangle(rect, 5.f * scaleMultiplier);

            melatonin::DropShadow shadow;
            shadow.setColor(juce::Colours::black.withAlpha(0.5f));
            shadow.setOffset(juce::roundToInt(5.f * scaleMultiplier),
                             juce::roundToInt(5.f * scaleMultiplier));
            shadow.setRadius(juce::roundToInt(8.f * scaleMultiplier));
            shadow.render(g, p);

            g.setColour(juce::Colours::white);
            g.fillRoundedRectangle(rect.toFloat(), 5.f * scaleMultiplier);

            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(rect.toFloat(), 5.f * scaleMultiplier,
                                   2.f * scaleMultiplier);

            g.drawFittedText(
                text,
                getLocalBounds().reduced(static_cast<int>(font.getHeight())),
                juce::Justification::centred, 4);
        }

    private:
        const std::function<juce::Font &()> &getMainFontScaled;
        const std::function<void()> deleteMe;
        float scaleMultiplier = 1.f;
    };
} // namespace vmpc_juce::gui::vector
