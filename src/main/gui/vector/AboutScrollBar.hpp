#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "TextWithLinks.hpp"
#include "VmpcJuceResourceUtil.hpp"

namespace vmpc_juce::gui::vector
{

    class AboutScrollBar : public juce::Component
    {
    public:
        AboutScrollBar(
            const std::function<float()> &getScaleToUse,
            const std::function<float()> getScrollOffsetFractionToUse,
            const std::function<void(float)> setScrollOffsetFractionToUse)
            : getScale(getScaleToUse),
              getScrollOffsetFraction(getScrollOffsetFractionToUse),
              setScrollOffsetFraction(setScrollOffsetFractionToUse)
        {
        }

        void paint(juce::Graphics &g) override
        {
            const auto scale = getScale();
            const auto radius = scale * 2.f;

            auto color = juce::Colours::black;

            if (mouseIsOverScrollBarRect)
            {
                color = color.brighter(0.8f);
            }

            g.setColour(color);
            g.fillRoundedRectangle(getScrollBarRect(), radius);
        }

        void mouseExit(const juce::MouseEvent &) override
        {
            mouseIsOverScrollBarRect = false;
            repaint();
        }

        void mouseDown(const juce::MouseEvent &e) override
        {
            if (getScrollBarRect().contains(e.position))
            {
                isDragging = true;
            }
        }

        void mouseMove(const juce::MouseEvent &e) override
        {
            const auto currentMouseIsOverScrollBarRect =
                mouseIsOverScrollBarRect;
            mouseIsOverScrollBarRect = getScrollBarRect().contains(e.position);

            if (mouseIsOverScrollBarRect == currentMouseIsOverScrollBarRect)
            {
                return;
            }

            repaint();
        }

        void mouseUp(const juce::MouseEvent &) override
        {
            isDragging = false;
            lastDy = 0;
        }

        void mouseDrag(const juce::MouseEvent &e) override
        {
            if (!isDragging)
            {
                return;
            }

            const auto distanceToProcess =
                e.getDistanceFromDragStartY() - lastDy;
            const auto scrollOffsetFraction =
                getScrollOffsetFraction() +
                (distanceToProcess /
                 (getHeight() - getScrollBarRect().getHeight()));
            setScrollOffsetFraction(scrollOffsetFraction);
            lastDy = e.getDistanceFromDragStartY();
        }

        const bool isCurrentlyDragging()
        {
            return isDragging;
        }

    private:
        juce::Rectangle<float> getScrollBarRect()
        {
            const auto scale = getScale();
            const auto rectHeight = scale * 15.f;
            const auto scrollOffset =
                getScrollOffsetFraction() * (getHeight() - rectHeight);
            return juce::Rectangle<float>(0, scrollOffset, getWidth(),
                                          rectHeight);
        }

        const std::function<float()> &getScale;
        const std::function<float()> getScrollOffsetFraction;
        const std::function<void(const float)> setScrollOffsetFraction;
        bool isDragging = false;
        int lastDy = 0;
        bool mouseIsOverScrollBarRect = false;
    };
} // namespace vmpc_juce::gui::vector
