#pragma once

#include "gui/vector/DisclaimerLayout.hpp"

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
            const auto layout = disclaimer_layout::calculate(
                getLocalBounds().toFloat(), getMainFontScaled(),
                scaleMultiplier);

            juce::Path p;
            p.addRoundedRectangle(layout.panelBounds, 5.f * layout.styleScale);

            melatonin::DropShadow shadow;
            shadow.setColor(juce::Colours::black.withAlpha(0.5f));
            shadow.setOffset(juce::roundToInt(5.f * layout.styleScale),
                             juce::roundToInt(5.f * layout.styleScale));
            shadow.setRadius(juce::roundToInt(8.f * layout.styleScale));
            shadow.render(g, p);

            g.setColour(juce::Colours::white);
            g.fillRoundedRectangle(layout.panelBounds, 5.f * layout.styleScale);

            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(layout.panelBounds, 5.f * layout.styleScale,
                                   2.f * layout.styleScale);

            layout.text.draw(g, layout.contentBounds);
        }

    private:
        const std::function<juce::Font &()> &getMainFontScaled;
        const std::function<void()> deleteMe;
        float scaleMultiplier = 1.f;
    };
} // namespace vmpc_juce::gui::vector
