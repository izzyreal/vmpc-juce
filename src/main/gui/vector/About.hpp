#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "TextWithLinks.hpp"
#include "VmpcJuceResourceUtil.hpp"

namespace vmpc_juce::gui::vector {

    const float marginAtScale1 = 8.f;

    class AboutScrollBar : public juce::Component {
        public:
            AboutScrollBar(const std::function<float()> &getScaleToUse, const std::function<float()> getScrollOffsetFractionToUse, const std::function<void(float)> setScrollOffsetFractionToUse) :
                getScale(getScaleToUse), getScrollOffsetFraction(getScrollOffsetFractionToUse), setScrollOffsetFraction(setScrollOffsetFractionToUse)
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
                const auto currentMouseIsOverScrollBarRect = mouseIsOverScrollBarRect;
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

                const auto distanceToProcess = e.getDistanceFromDragStartY() - lastDy;
                const auto scrollOffsetFraction = getScrollOffsetFraction() + (distanceToProcess / (getHeight() - getScrollBarRect().getHeight()));
                setScrollOffsetFraction(scrollOffsetFraction);
                lastDy = e.getDistanceFromDragStartY();
            }

        private:
            juce::Rectangle<float> getScrollBarRect()
            {
                const auto scale = getScale();
                const auto rectHeight = scale * 15.f;
                const auto scrollOffset = getScrollOffsetFraction() * (getHeight() - rectHeight);
                const auto radius = scale * 2.f;
                return juce::Rectangle<float>(0, scrollOffset, getWidth(), rectHeight);
            }

            const std::function<float()> &getScale;
            const std::function<float()> getScrollOffsetFraction;
            const std::function<void(const float)> setScrollOffsetFraction;
            bool isDragging = false;
            int lastDy = 0;
            bool mouseIsOverScrollBarRect = false;
    };

    class CloseAbout : public juce::Component {
        public:
            CloseAbout(const std::function<float()> &getScaleToUse, const std::function<void()> &closeAboutToUse)
                : getScale(getScaleToUse), closeAbout(closeAboutToUse)
            {
            }

            void paint(juce::Graphics &g) override
            {
                const auto lineThickness = 1.f * getScale();
                const auto rect = getLocalBounds().toFloat().reduced(lineThickness * 3.f);
                auto color = juce::Colours::black;

                if (mouseIsOver) color = color.brighter(0.8f);

                g.setColour(color);

                g.drawLine(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), lineThickness);
                g.drawLine(rect.getWidth(), rect.getY(), rect.getX(), rect.getHeight(), lineThickness);
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

            void mouseDown(const juce::MouseEvent &e) override
            {
                closeAbout();
            }

        private:
            const std::function<float()> &getScale;
            const std::function<void()> closeAbout;
            bool mouseIsOver = false;
    };

    class AboutBorder : public juce::Component {
        public:
            AboutBorder(const std::function<float()> &getScaleToUse) : getScale(getScaleToUse)
        {
            setInterceptsMouseClicks(false, false);
        }
            void paint(juce::Graphics &g)
            {
                const auto scale = getScale();
                const auto lineThickness = scale;
                const auto radius = scale * 3;
                const auto margin = scale * marginAtScale1;
                const auto rect = getLocalBounds().toFloat().reduced(lineThickness * 0.5).withTrimmedLeft(radius).withTrimmedRight(radius);
                const auto top = rect.withTrimmedBottom(getHeight() - margin);
                const auto bottom = rect.withTrimmedTop(getHeight() - margin);

                g.setColour(juce::Colours::white);
                g.fillRect(top); g.fillRect(bottom);

                const auto rect2 = getLocalBounds().toFloat().reduced(lineThickness * 0.5);

                g.setColour(juce::Colours::black);
                g.drawRoundedRectangle(rect2, radius, lineThickness);
            }

        private:
            const std::function<float()> &getScale;
    };

    class About : public juce::Component {
        public:
            About(const std::function<float()> &getScaleToUse, const std::function<juce::Font&()> &getNimbusSansScaledToUse, const std::function<void()> &closeAboutToUse)
                : getScale(getScaleToUse), getNimbusSansScaled(getNimbusSansScaledToUse)
            {
                aboutBorder = new AboutBorder(getScale);
                const auto creditsTextData = vmpc_juce::VmpcJuceResourceUtil::getResourceData("txt/credits.txt");
                creditsText = std::string(creditsTextData.begin(), creditsTextData.end());
                textWithLinks = new TextWithLinks(creditsText, getNimbusSansScaled);
                addAndMakeVisible(textWithLinks);
                addAndMakeVisible(aboutBorder);

                closeAbout = new CloseAbout(getScale, closeAboutToUse);
                addAndMakeVisible(closeAbout);

                const auto setScrollOffsetFraction = [this](const float fr) {
                    setScrollOffset(fr * maxScrollOffset);
                };

                aboutScrollBar = new AboutScrollBar(getScale, [this]{ return scrollOffset / maxScrollOffset; }, setScrollOffsetFraction);
                addAndMakeVisible(aboutScrollBar);
            }

            void paint(juce::Graphics &g) override
            {
                const auto scale = getScale();
                const auto rect = getLocalBounds().toFloat().reduced(scale);

                g.setColour(juce::Colours::white);
                g.fillRoundedRectangle(rect, scale);
            }

            void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w) override
            {
                const auto oldScrollOffset = scrollOffset;

                scrollOffset = std::clamp<float>(scrollOffset - (150 * w.deltaY), 0, maxScrollOffset);

                if (scrollOffset == oldScrollOffset)
                {
                    return;
                }

                resized();
                repaint();
            }

            void resized() override
            {
                aboutBorder->setBounds(0, 0, getWidth(), getHeight());

                const auto scale = getScale();

                const auto scrollBarWidth = scale * 4.f;

                const auto closeAboutWidth = getWidth() * 0.06;
                const auto closeAboutMargin = scale * 1.f;

                const auto closeAboutRect = juce::Rectangle<float>(getWidth() - closeAboutWidth + scale, 0, closeAboutWidth, closeAboutWidth)
                    .translated(-closeAboutMargin, closeAboutMargin * 2);

                closeAbout->setBounds(closeAboutRect.toNearestInt());

                const auto margin = marginAtScale1 * scale;

                aboutScrollBar->setBounds(getWidth() - (scrollBarWidth + margin) + (scale * 2.f), closeAboutWidth, scrollBarWidth, getHeight() - ((margin * 0.5) + closeAboutWidth));

                textWithLinks->setBounds(margin, margin, getWidth() - std::ceil(margin * 2.f), 10000);

                const auto newTextHeight = textWithLinks->getHeight();
                const auto newMaxScrollOffset = newTextHeight - (getHeight() - (margin * 2));

                if (scrollOffset != 0.f)
                {
                    const auto resizeFactor = newMaxScrollOffset / maxScrollOffset;
                    scrollOffset = scrollOffset * resizeFactor;
                    textWithLinks->setTopLeftPosition(margin, margin - scrollOffset);
                }

                maxScrollOffset = newMaxScrollOffset;
            }

            ~About() override
            {
                delete aboutBorder;
                delete closeAbout;
                delete textWithLinks;
                delete aboutScrollBar;
            }

        private:
            void setScrollOffset(float newScrollOffset)
            {
                const auto oldScrollOffset = scrollOffset;

                scrollOffset = std::clamp<float>(newScrollOffset, 0, maxScrollOffset);

                if (scrollOffset == oldScrollOffset)
                {
                    return;
                }

                resized();
                repaint();
            }

            const std::function<float()> &getScale;
            const std::function<juce::Font&()> &getNimbusSansScaled;
            TextWithLinks *textWithLinks = nullptr;
            std::string creditsText;
            float scrollOffset = 0.f;
            float maxScrollOffset = 0.f;
            int previousHeight = 0.f;
            juce::Component *aboutBorder = nullptr;
            juce::Component *closeAbout = nullptr;
            juce::Component *aboutScrollBar = nullptr;
    };
} // namespace vmpc_juce::gui::vector