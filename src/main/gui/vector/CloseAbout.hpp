#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "TextWithLinks.hpp"
#include "VmpcJuceResourceUtil.hpp"

namespace vmpc_juce::gui::vector
{

    class CloseAbout : public juce::Component
    {
    public:
        CloseAbout(const std::function<float()> &getScaleToUse,
                   const std::function<void()> &closeAboutToUse)
            : getScale(getScaleToUse), closeAbout(closeAboutToUse)
        {
        }

        void paint(juce::Graphics &g) override
        {
            const auto lineThickness = 1.f * getScale();
            const auto rect =
                getLocalBounds().toFloat().reduced(lineThickness * 3.f);
            auto color = juce::Colours::black;

            if (mouseIsOver)
            {
                color = color.brighter(0.8f);
            }

            g.setColour(color);

            g.drawLine(rect.getX(), rect.getY(), rect.getWidth(),
                       rect.getHeight(), lineThickness);
            g.drawLine(rect.getWidth(), rect.getY(), rect.getX(),
                       rect.getHeight(), lineThickness);
        }

        void mouseEnter(const juce::MouseEvent &) override
        {
            mouseIsOver = true;
            repaint();
        }

        void mouseExit(const juce::MouseEvent &) override
        {
            mouseIsOver = false;
            repaint();
        }

        void mouseDown(const juce::MouseEvent &) override
        {
            closeAbout();
        }

    private:
        const std::function<float()> &getScale;
        const std::function<void()> closeAbout;
        bool mouseIsOver = false;
    };
} // namespace vmpc_juce::gui::vector
