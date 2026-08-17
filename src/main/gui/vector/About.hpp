#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AboutBorder.hpp"
#include "AboutScrollBar.hpp"
#include "CloseAbout.hpp"

#include "TextWithLinks.hpp"
#include "VmpcJuceResourceUtil.hpp"

#include "FloatUtil.hpp"

namespace vmpc_juce::gui::vector
{

    class OutsideAboutMouseClickListener : public juce::MouseListener
    {
    public:
        std::function<void(const juce::MouseEvent &)> mouseDownFn;
        void mouseDown(const juce::MouseEvent &e) override
        {
            mouseDownFn(e);
        }
    };

    class AboutViewport : public juce::Viewport
    {
    public:
        std::function<void()> visibleAreaChangedFn;

        void visibleAreaChanged(const juce::Rectangle<int> &) override
        {
            if (visibleAreaChangedFn)
            {
                visibleAreaChangedFn();
            }
        }
    };

    class About : public juce::Component, juce::Timer
    {
    public:
        About(const std::function<float()> &getScaleToUse,
              const std::function<juce::Font &()> &getMainFontScaledToUse,
              const std::function<void()> &closeAboutToUse,
              const std::string wrapperTypeString)
            : getScale(getScaleToUse),
              getMainFontScaled(getMainFontScaledToUse),
              getAboutScale(
                  [this]
                  {
                      return getScale() * scaleMultiplier;
                  }),
              getAboutFontScaled(
                  [this]() -> juce::Font &
                  {
                      auto &font = getMainFontScaled();
                      font.setHeight(font.getHeight() * scaleMultiplier *
                                     fontScaleMultiplier);
                      return font;
                  }),
              closeAboutFn(closeAboutToUse)
        {
            aboutBorder = new AboutBorder(getAboutScale);
            const auto creditsTextData =
                vmpc_juce::VmpcJuceResourceUtil::getResourceData(
                    "txt/credits.txt");
            creditsText =
                std::string(creditsTextData.begin(), creditsTextData.end());

            replaceFormatPlaceHolder(creditsText, wrapperTypeString);

            textWithLinks = new TextWithLinks(creditsText, getAboutFontScaled);
            textViewport = new AboutViewport();
            textViewport->visibleAreaChangedFn = [this]
            {
                viewportVisibleAreaChanged();
            };
            textViewport->setViewedComponent(textWithLinks, false);
            textViewport->setScrollBarsShown(false, false, true, false);
#if JUCE_IOS
            textViewport->setScrollOnDragMode(
                juce::Viewport::ScrollOnDragMode::nonHover);
#else
            textViewport->setScrollOnDragMode(
                juce::Viewport::ScrollOnDragMode::never);
#endif
            addAndMakeVisible(textViewport);
            addAndMakeVisible(aboutBorder);

            closeAbout = new CloseAbout(getAboutScale, closeAboutToUse);
            addAndMakeVisible(closeAbout);

            const auto setScrollOffsetFraction = [this](const float fr)
            {
                const auto maxScrollOffset = getMaxScrollOffset();
                textViewport->setViewPosition(
                    0, juce::roundToInt(std::clamp(fr, 0.f, 1.f) *
                                        static_cast<float>(maxScrollOffset)));
            };

#if JUCE_IOS
            constexpr auto scrollBarInteractive = false;
#else
            constexpr auto scrollBarInteractive = true;
#endif
            aboutScrollBar = new AboutScrollBar(
                getAboutScale,
                [this]
                {
                    const auto maxScrollOffset = getMaxScrollOffset();
                    return maxScrollOffset > 0
                               ? static_cast<float>(
                                     textViewport->getViewPositionY()) /
                                     static_cast<float>(maxScrollOffset)
                               : 0.f;
                },
                [this]
                {
                    const auto textHeight = textWithLinks->getHeight();
                    return textHeight > 0
                               ? std::min(
                                     1.f,
                                     static_cast<float>(
                                         textViewport->getHeight()) /
                                         static_cast<float>(textHeight))
                               : 1.f;
                },
                setScrollOffsetFraction, scrollBarInteractive);
#if JUCE_IOS
            aboutScrollBar->setAlpha(0.f);
#endif
            addAndMakeVisible(aboutScrollBar);
            setInterceptsMouseClicks(true, false);
            startTimer(100);
        }

        void setScaleMultipliers(const float multiplier,
                                 const float fontMultiplier)
        {
            if (nearlyEqual(scaleMultiplier, multiplier) &&
                nearlyEqual(fontScaleMultiplier, fontMultiplier))
            {
                return;
            }

            scaleMultiplier = multiplier;
            fontScaleMultiplier = fontMultiplier;
            resized();
            repaint();
        }

        void timerCallback() override
        {
            if (!globalMouseListenerConfigured)
            {
                const auto mouseDownFn = [this](const juce::MouseEvent &e)
                {
                    if (!aboutBorder->getLocalBounds().contains(
                            e.getEventRelativeTo(this).getPosition()))
                    {
                        closeAboutFn();
                    }
                };

                outsideAboutMouseClickListener =
                    new OutsideAboutMouseClickListener();
                outsideAboutMouseClickListener->mouseDownFn = mouseDownFn;
                juce::Desktop::getInstance().addGlobalMouseListener(
                    outsideAboutMouseClickListener);
                globalMouseListenerConfigured = true;
            }

#if JUCE_IOS
            updateScrollIndicatorAlpha();
#else
            stopTimer();
#endif
        }

        void paint(juce::Graphics &g) override
        {
            const auto scale = getAboutScale();
            const auto rect = getLocalBounds().toFloat().reduced(scale);

            g.setColour(juce::Colours::white);
            g.fillRoundedRectangle(rect, scale);
        }

        void resized() override
        {
            aboutBorder->setBounds(0, 0, getWidth(), getHeight());

            const auto scale = getAboutScale();
            const auto oldMaxScrollOffset = getMaxScrollOffset();
            const auto oldScrollFraction =
                oldMaxScrollOffset > 0
                    ? static_cast<float>(textViewport->getViewPositionY()) /
                          static_cast<float>(oldMaxScrollOffset)
                    : 0.f;

            const auto scrollBarWidth = scale * 4.f;

            const auto closeAboutWidth = getWidth() * 0.06;
            const auto closeAboutMargin = scale * 1.f;

            const auto closeAboutRect =
                juce::Rectangle<double>(getWidth() - closeAboutWidth + scale, 0,
                                        closeAboutWidth, closeAboutWidth)
                    .translated(-closeAboutMargin, closeAboutMargin * 2);

            closeAbout->setBounds(closeAboutRect.toNearestInt());

            const auto margin = marginAtScale1 * scale;
            const auto marginInt = juce::roundToInt(margin);

#if JUCE_IOS
            layoutInProgress = true;
#endif
            textViewport->setBounds(getLocalBounds().reduced(marginInt));

            const auto textWidth = std::max(1, textViewport->getWidth());
            textWithLinks->setBounds(0, 0, textWidth, 1);
            textWithLinks->updateFont();
            const auto newTextHeight = textWithLinks->getTextLayoutHeight();
            textWithLinks->setBounds(
                0, 0, textWidth,
                std::max(newTextHeight, textViewport->getHeight()));

            const auto newMaxScrollOffset = getMaxScrollOffset();
            textViewport->setViewPosition(
                0, juce::roundToInt(oldScrollFraction *
                                    static_cast<float>(newMaxScrollOffset)));
#if JUCE_IOS
            layoutInProgress = false;
            lastViewportPositionY = textViewport->getViewPositionY();
#endif

            aboutScrollBar->setBounds(
                static_cast<int>(static_cast<float>(getWidth()) -
                                 (scrollBarWidth + margin) + (scale * 2.f)),
                static_cast<int>(closeAboutWidth),
                static_cast<int>(scrollBarWidth),
                static_cast<int>(getHeight() -
                                 ((margin * 0.5) + closeAboutWidth)));
            aboutScrollBar->setVisible(newMaxScrollOffset > 0);
#if JUCE_IOS
            if (newMaxScrollOffset <= 0)
            {
                aboutScrollBar->setAlpha(0.f);
            }
#else
            aboutScrollBar->setAlpha(1.f);
#endif
            aboutScrollBar->repaint();
        }

        ~About() override
        {
            juce::Desktop::getInstance().removeGlobalMouseListener(
                outsideAboutMouseClickListener);
            delete outsideAboutMouseClickListener;
            delete aboutBorder;
            delete closeAbout;
            textViewport->visibleAreaChangedFn = nullptr;
            delete textViewport;
            delete aboutScrollBar;
            delete textWithLinks;
        }

    private:
        int getMaxScrollOffset() const
        {
            return std::max(0, textWithLinks->getHeight() -
                                   textViewport->getHeight());
        }

        void viewportVisibleAreaChanged()
        {
            if (aboutScrollBar == nullptr)
            {
                return;
            }

            aboutScrollBar->repaint();
#if JUCE_IOS
            const auto currentPositionY = textViewport->getViewPositionY();
            if (!layoutInProgress &&
                currentPositionY != lastViewportPositionY &&
                getMaxScrollOffset() > 0)
            {
                lastScrollActivityMs =
                    juce::Time::getMillisecondCounterHiRes();
                aboutScrollBar->setAlpha(1.f);
                startTimerHz(30);
            }
            lastViewportPositionY = currentPositionY;
#endif
        }

#if JUCE_IOS
        void updateScrollIndicatorAlpha()
        {
            if (lastScrollActivityMs <= 0.0)
            {
                if (globalMouseListenerConfigured)
                {
                    stopTimer();
                }
                return;
            }

            constexpr auto fadeDelayMs = 500.0;
            constexpr auto fadeDurationMs = 250.0;
            const auto elapsedMs =
                juce::Time::getMillisecondCounterHiRes() -
                lastScrollActivityMs;
            if (elapsedMs <= fadeDelayMs)
            {
                return;
            }

            const auto alpha = static_cast<float>(std::clamp(
                1.0 - ((elapsedMs - fadeDelayMs) / fadeDurationMs), 0.0, 1.0));
            aboutScrollBar->setAlpha(alpha);
            if (alpha <= 0.f)
            {
                lastScrollActivityMs = 0.0;
                if (globalMouseListenerConfigured)
                {
                    stopTimer();
                }
            }
        }
#endif

        void replaceFormatPlaceHolder(std::string &rawText,
                                      const std::string format)
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
        const std::function<juce::Font &()> &getMainFontScaled;
        float scaleMultiplier = 1.f;
        float fontScaleMultiplier = 1.f;
        const std::function<float()> getAboutScale;
        const std::function<juce::Font &()> getAboutFontScaled;
        const std::function<void()> &closeAboutFn;
        TextWithLinks *textWithLinks = nullptr;
        AboutViewport *textViewport = nullptr;
        std::string creditsText;
        juce::Component *aboutBorder = nullptr;
        juce::Component *closeAbout = nullptr;
        AboutScrollBar *aboutScrollBar = nullptr;
        OutsideAboutMouseClickListener *outsideAboutMouseClickListener =
            nullptr;
#if JUCE_IOS
        int lastViewportPositionY = 0;
        double lastScrollActivityMs = 0.0;
        bool layoutInProgress = false;
#endif
        bool globalMouseListenerConfigured = false;
    };
} // namespace vmpc_juce::gui::vector
