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
            const std::function<float()> getVisibleFractionToUse,
            const std::function<void(float)> setScrollOffsetFractionToUse,
            const bool interactiveToUse)
            : getScale(getScaleToUse),
              getScrollOffsetFraction(getScrollOffsetFractionToUse),
              getVisibleFraction(getVisibleFractionToUse),
              setScrollOffsetFraction(setScrollOffsetFractionToUse),
              interactive(interactiveToUse)
        {
            setInterceptsMouseClicks(interactive, false);
        }

        void paint(juce::Graphics &g) override
        {
            const auto scale = getScale();
            const auto radius = scale * 2.f;

            auto color = juce::Colours::black;

            if (interactive && mouseIsOverScrollBarRect)
            {
                color = color.brighter(0.8f);
            }

            const auto scrollBarRect = getScrollBarRect();
            if (scrollBarRect.isEmpty())
            {
                return;
            }

            g.setColour(color);
            g.fillRoundedRectangle(scrollBarRect, radius);
        }

        void mouseExit(const juce::MouseEvent &) override
        {
            mouseIsOverScrollBarRect = false;
            repaint();
        }

        void mouseDown(const juce::MouseEvent &e) override
        {
            if (interactive && getScrollBarRect().contains(e.position))
            {
                isDragging = true;
            }
        }

        void mouseMove(const juce::MouseEvent &e) override
        {
            if (!interactive)
            {
                return;
            }

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
            const auto availableTravel =
                std::max(1.f, static_cast<float>(getHeight()) -
                                  getScrollBarRect().getHeight());
            const auto scrollOffsetFraction =
                getScrollOffsetFraction() +
                (static_cast<float>(distanceToProcess) / availableTravel);
            setScrollOffsetFraction(scrollOffsetFraction);
            lastDy = e.getDistanceFromDragStartY();
        }

    private:
        juce::Rectangle<float> getScrollBarRect()
        {
            if (getHeight() <= 0)
            {
                return {};
            }

            const auto scale = getScale();
            const auto visibleFraction =
                std::clamp(getVisibleFraction(), 0.f, 1.f);
            if (visibleFraction >= 1.f)
            {
                return {};
            }

            const auto rectHeight = std::clamp(
                static_cast<float>(getHeight()) * visibleFraction,
                scale * 15.f, static_cast<float>(getHeight()));
            const auto scrollOffset =
                std::clamp(getScrollOffsetFraction(), 0.f, 1.f) *
                (static_cast<float>(getHeight()) - rectHeight);
            return juce::Rectangle<float>(0, scrollOffset, getWidth(),
                                          rectHeight);
        }

        const std::function<float()> &getScale;
        const std::function<float()> getScrollOffsetFraction;
        const std::function<float()> getVisibleFraction;
        const std::function<void(const float)> setScrollOffsetFraction;
        const bool interactive;
        bool isDragging = false;
        int lastDy = 0;
        bool mouseIsOverScrollBarRect = false;
    };
} // namespace vmpc_juce::gui::vector
