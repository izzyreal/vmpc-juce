#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AboutBorder.hpp"
#include "AboutScrollBar.hpp"
#include "CloseAbout.hpp"

#include "TextWithLinks.hpp"
#include "VmpcJuceResourceUtil.hpp"

#include "FloatUtil.hpp"

namespace vmpc_juce::gui::vector {

    class OutsideAboutMouseClickListener : public juce::MouseListener {
        public:
            std::function<void(const juce::MouseEvent&)> mouseDownFn;
            void mouseDown(const juce::MouseEvent &e) override
            {
                mouseDownFn(e);
            }
    };

    class About : public juce::Component, juce::Timer {
        public:
            About(const std::function<float()> &getScaleToUse,
                  const std::function<juce::Font&()> &getMainFontScaledToUse,
                  const std::function<void()> &closeAboutToUse,
                  const std::string wrapperTypeString)
                : getScale(getScaleToUse), getMainFontScaled(getMainFontScaledToUse), closeAboutFn(closeAboutToUse)
            {
                aboutBorder = new AboutBorder(getScale);
                const auto creditsTextData = vmpc_juce::VmpcJuceResourceUtil::getResourceData("txt/credits.txt");
                creditsText = std::string(creditsTextData.begin(), creditsTextData.end());

                replaceFormatPlaceHolder(creditsText, wrapperTypeString);

                textWithLinks = new TextWithLinks(creditsText, getMainFontScaled);
                addAndMakeVisible(textWithLinks);
                addAndMakeVisible(aboutBorder);

                closeAbout = new CloseAbout(getScale, closeAboutToUse);
                addAndMakeVisible(closeAbout);

                const auto setScrollOffsetFraction = [this](const float fr) {
                    setScrollOffset(fr * maxScrollOffset);
                };

                aboutScrollBar = new AboutScrollBar(getScale, [this]{ return scrollOffset / maxScrollOffset; }, setScrollOffsetFraction);
                addAndMakeVisible(aboutScrollBar);
                setInterceptsMouseClicks(true, false);
                startTimer(100);
            }

            void timerCallback() override
            {
                if (!globalMouseListenerConfigured)
                {
                    
                    const auto mouseDownFn = [&](const juce::MouseEvent &e){
                        if (!aboutBorder->getLocalBounds().contains(e.getEventRelativeTo(this).getPosition()))
                        {
                            closeAboutFn();
                        }
                    };
                    
                    outsideAboutMouseClickListener = new OutsideAboutMouseClickListener();
                    outsideAboutMouseClickListener->mouseDownFn = mouseDownFn;
                    juce::Desktop::getInstance().addGlobalMouseListener(outsideAboutMouseClickListener);
                    globalMouseListenerConfigured = true;
                    stopTimer();
                    return;
                }

                if (scrollAmountForTimer == 0)
                {
                    stopTimer();
                    return;
                }

                setScrollOffset(scrollOffset + static_cast<float>(scrollAmountForTimer * 7));
                textWithLinks->updateSelectionEnd(textWithLinks->getMouseXYRelative());
            }

            juce::Rectangle<float> getVisualTextBounds()
            {
                const auto scale = getScale();
                const auto lineThickness = scale;
                const auto radius = scale * 3;
                const auto margin = scale * marginAtScale1;
                auto rect = getLocalBounds().toFloat().reduced(lineThickness * 0.5f).withTrimmedLeft(radius).withTrimmedRight(radius);
                rect = rect.withTrimmedBottom(margin);
                rect = rect.withTrimmedTop(margin);
                return rect;
            }

            void mouseDrag(const juce::MouseEvent &e) override
            {
                if (aboutScrollBar->isCurrentlyDragging())
                {
                    return;
                }

                const auto textBounds = getVisualTextBounds();

                const bool increaseScrollOffset = static_cast<float>(e.getPosition().getY()) > textBounds.getBottom();
                const bool decreaseScrollOffset = static_cast<float>(e.getPosition().getY()) < textBounds.getY();
                const int lengthOfAreaThatAffectsScrollSpeed = 100;

                if (increaseScrollOffset)
                {
                    const int distanceBetweenMouseAndTextBoundsBottom = std::min<int>(e.getPosition().getY() - static_cast<int>(textBounds.getBottom()), lengthOfAreaThatAffectsScrollSpeed);
                    const int interval = static_cast<int>((lengthOfAreaThatAffectsScrollSpeed - (static_cast<float>(distanceBetweenMouseAndTextBoundsBottom) + 75.f)) * 6.f);

                    scrollAmountForTimer = 1;

                    if (isTimerRunning() && getTimerInterval() != interval)
                    {
                        stopTimer();
                        startTimer(interval);
                    }
                    else if (!isTimerRunning())
                    {
                        startTimer(interval);
                    }

                    return;
                }

                if (decreaseScrollOffset)
                {
                    const int distanceBetweenMouseAndTextBoundsTop = std::min<int>(static_cast<int>(textBounds.getY()) - e.getPosition().getY(), lengthOfAreaThatAffectsScrollSpeed);
                    const int interval = static_cast<int>((lengthOfAreaThatAffectsScrollSpeed - (static_cast<float>(distanceBetweenMouseAndTextBoundsTop) + 75.f)) * 6.f);

                    scrollAmountForTimer = -1;

                    if (isTimerRunning() && getTimerInterval() != interval)
                    {
                        stopTimer();
                        startTimer(interval);
                    }
                    else if (!isTimerRunning())
                    {
                        startTimer(interval);
                    }

                    return;
                }

                if (isTimerRunning())
                {
                    stopTimer();
                }
                
                textWithLinks->mouseDrag(e.getEventRelativeTo(textWithLinks));
            }

            void mouseDown(const juce::MouseEvent &e) override
            {
                textWithLinks->mouseDown(e.getEventRelativeTo(textWithLinks));
            }

            void mouseDoubleClick(const juce::MouseEvent &e) override
            {
                textWithLinks->mouseDoubleClick(e.getEventRelativeTo(textWithLinks));
            }

            void mouseUp(const juce::MouseEvent &e) override
            {
                if (isTimerRunning()) stopTimer();
                textWithLinks->mouseUp(e.getEventRelativeTo(textWithLinks));
            }

            void mouseMove(const juce::MouseEvent &e) override
            {
                textWithLinks->mouseMove(e.getEventRelativeTo(textWithLinks));
            }

            void paint(juce::Graphics &g) override
            {
                const auto scale = getScale();
                const auto rect = getLocalBounds().toFloat().reduced(scale);

                g.setColour(juce::Colours::white);
                g.fillRoundedRectangle(rect, scale);
            }

            void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &w) override
            {
                setScrollOffset(scrollOffset - (150 * w.deltaY));
            }

            void resized() override
            {
                aboutBorder->setBounds(0, 0, getWidth(), getHeight());

                const auto scale = getScale();

                const auto scrollBarWidth = scale * 4.f;

                const auto closeAboutWidth = getWidth() * 0.06;
                const auto closeAboutMargin = scale * 1.f;

                const auto closeAboutRect = juce::Rectangle<double>(getWidth() - closeAboutWidth + scale, 0, closeAboutWidth, closeAboutWidth)
                    .translated(-closeAboutMargin, closeAboutMargin * 2);

                closeAbout->setBounds(closeAboutRect.toNearestInt());

                const auto margin = marginAtScale1 * scale;

                aboutScrollBar->setBounds(static_cast<int>(static_cast<float>(getWidth()) - (scrollBarWidth + margin) + (scale * 2.f)), static_cast<int>(closeAboutWidth), static_cast<int>(scrollBarWidth), static_cast<int>(getHeight() - ((margin * 0.5) + closeAboutWidth)));

                if (textWithLinks->getWidth() == 0)
                {
                    textWithLinks->setBounds(static_cast<int>(margin), static_cast<int>(margin), static_cast<int>(static_cast<float>(getWidth()) - std::ceil(margin * 2.f)), 100000);
                }

                textWithLinks->updateFont();
                const auto newTextHeight = textWithLinks->getTextLayoutHeight();
                textWithLinks->setBounds(static_cast<int>(margin), static_cast<int>(margin), static_cast<int>(static_cast<float>(getWidth()) - std::ceil(margin * 2.f)), newTextHeight);

                const auto newMaxScrollOffset = static_cast<float>(newTextHeight) - (static_cast<float>(getHeight()) - (margin * 2));

                if (scrollOffset != 0.f)
                {
                    const auto resizeFactor = newMaxScrollOffset / maxScrollOffset;
                    setScrollOffset(scrollOffset * resizeFactor, true);
                }

                maxScrollOffset = newMaxScrollOffset;
            }

            ~About() override
            {
                juce::Desktop::getInstance().removeGlobalMouseListener(outsideAboutMouseClickListener);
                delete outsideAboutMouseClickListener;
                delete aboutBorder;
                delete closeAbout;
                delete textWithLinks;
                delete aboutScrollBar;
            }
        
        private:
            void setScrollOffset(float newScrollOffset, bool force = false)
            {
                const auto oldScrollOffset = scrollOffset;

                scrollOffset = std::clamp<float>(newScrollOffset, 0, maxScrollOffset);

                if (nearlyEqual(scrollOffset, oldScrollOffset) && !force)
                {
                    return;
                }

                const auto scale = getScale();
                const auto margin = marginAtScale1 * scale;

                textWithLinks->setTopLeftPosition(static_cast<int>(margin), static_cast<int>(margin - scrollOffset));
                repaint();
            }

            void replaceFormatPlaceHolder(std::string &rawText, const std::string format)
            {
                static const std::string formatPlaceHolder = "<format>";
                const size_t pos = rawText.find(formatPlaceHolder);
                
                if (pos != std::string::npos)
                {
                    rawText.replace(pos, formatPlaceHolder.length(), format);
                }
            }

            const float marginAtScale1 = 8.f;

            const std::function<float()> &getScale;
            const std::function<juce::Font&()> &getMainFontScaled;
            const std::function<void()> closeAboutFn;
            TextWithLinks *textWithLinks = nullptr;
            std::string creditsText;
            float scrollOffset = 0.f;
            float maxScrollOffset = 0.f;
            juce::Component *aboutBorder = nullptr;
            juce::Component *closeAbout = nullptr;
            AboutScrollBar *aboutScrollBar = nullptr;
            OutsideAboutMouseClickListener *outsideAboutMouseClickListener = nullptr;
            int scrollAmountForTimer = 0;
            bool globalMouseListenerConfigured = false;
    };
} // namespace vmpc_juce::gui::vector
