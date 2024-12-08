#pragma once

#include "melatonin_blur/melatonin/shadows.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur/melatonin_blur.h>

namespace vmpc_juce::gui::vector {

    class InfoTooltip : public juce::Component {
        public:
            InfoTooltip(
                    const std::function<float()> &getScaleToUse,
                    const std::function<juce::Font&()> &getNimbusSansScaledToUse,
                    juce::Component *tooltipOverlayToUse)
                : getScale(getScaleToUse),
                getNimbusSansScaled(getNimbusSansScaledToUse),
                tooltipOverlay(tooltipOverlayToUse)
        {
            shadow.setColor(juce::Colours::black.withAlpha(0.6f));
        }

            void configure(
                    const std::string tooltipTextToUse,
                    const juce::Component *anchorToUse)
            {
                tooltipText = tooltipTextToUse;
                anchor = anchorToUse;
                resized();
                repaint();
            }

            void paint(juce::Graphics &g) override
            {
                const auto scale = getScale();
                const auto radius = 1.f * scale;
                const auto lineThickness = .5f * scale;

                auto arrowHeight = getArrowHeightScaled();
                auto rect = getLocalBounds().toFloat();

                rect.reduce(shadowMargin, shadowMargin);

                rect = rect.withTrimmedTop(arrowHeight);

                rect.reduce(lineThickness, lineThickness);

                const auto triangleWidth = arrowHeight;
                const auto rectTopY = rect.getY();
                const auto rectBottomY = rect.getBottom();
                const auto rectLeftX = rect.getX();
                const auto rectRightX = rect.getRight();
                const auto arrowX = tooltipIsPositionedBelowAnchor ? getUpArrowTipPosWithinSelf().x : getDownArrowTipPosWithinSelf().x;

                juce::Path path;

                if (tooltipIsPositionedBelowAnchor)
                {
                    path.startNewSubPath(arrowX - triangleWidth / 2, rectTopY);
                    path.lineTo(arrowX, rectTopY - arrowHeight);
                    path.lineTo(arrowX + triangleWidth / 2, rectTopY);
                    path.lineTo(rectRightX - radius, rectTopY);
                    path.addArc(rectRightX - radius * 2, rectTopY, radius * 2, radius * 2, 0, juce::MathConstants<float>::pi * 0.5f);
                    path.lineTo(rectRightX, rectBottomY - radius);
                    path.addArc(rectRightX - radius * 2, rectBottomY - radius * 2, radius * 2, radius * 2, juce::MathConstants<float>::pi * 0.5f, juce::MathConstants<float>::pi);
                    path.lineTo(rectLeftX + radius, rectBottomY);
                    path.addArc(rectLeftX, rectBottomY - radius * 2, radius * 2, radius * 2, juce::MathConstants<float>::pi, juce::MathConstants<float>::pi * 1.5f);
                    path.lineTo(rectLeftX, rectTopY + radius);
                    path.addArc(rectLeftX, rectTopY, radius * 2, radius * 2, juce::MathConstants<float>::pi * 1.5f, juce::MathConstants<float>::twoPi);
                }
                else
                {
                    path.startNewSubPath(arrowX - triangleWidth / 2, rectBottomY);
                    path.lineTo(arrowX, rectBottomY + arrowHeight);
                    path.lineTo(arrowX + triangleWidth / 2, rectBottomY);
                    path.lineTo(rectRightX - radius, rectBottomY);
                    path.addArc(rectRightX - radius * 2, rectBottomY - radius * 2, radius * 2, radius * 2, juce::MathConstants<float>::pi, juce::MathConstants<float>::pi * 0.5f);
                    path.lineTo(rectRightX, rectTopY + radius);
                    path.addArc(rectRightX - radius * 2, rectTopY, radius * 2, radius * 2, juce::MathConstants<float>::pi * 0.5f, 0);
                    path.lineTo(rectLeftX + radius, rectTopY);
                    path.addArc(rectLeftX, rectTopY, radius * 2, radius * 2, 0, juce::MathConstants<float>::pi * -0.5f);
                    path.lineTo(rectLeftX, rectBottomY - radius);
                    path.addArc(rectLeftX, rectBottomY - radius * 2, radius * 2, radius * 2, juce::MathConstants<float>::pi * -0.5f, -juce::MathConstants<float>::pi);
                }

                path.closeSubPath();

                shadow.setOffset(2 * scale, 2 * scale);
                shadow.setRadius(scale * 3);
                shadow.setSpread(scale);
                shadow.render(g, path);

                g.setColour(juce::Colours::white);
                g.fillPath(path);

                g.setColour(juce::Colours::black);
                g.strokePath(path, juce::PathStrokeType(lineThickness));
                g.setFont(getFont());
                g.drawText(tooltipText, rect, juce::Justification::centred);
            }

            void resized() override
            {
                const auto scale = getScale();
                const auto textWidth = getTextWidth();
                const auto textHeight = getFont().getHeight();
                const auto margin = 3.f * scale;
                const auto textWidthWithMargin = textWidth + (margin * 2);
                const auto textHeightWithMargin = textHeight + (margin * 2);
                const auto tooltipHeight = textHeightWithMargin + getArrowHeightScaled();

                const auto tooltipOverlayBounds = tooltipOverlay->getBounds();

                auto arrowTipPos = getUpArrowTipPosWithinTooltipOverlay();
                auto tooltipX = (arrowTipPos.x - (textWidthWithMargin * 0.5f));
                auto tooltipY = arrowTipPos.y;
                auto rect = juce::Rectangle<int>(tooltipX, tooltipY, textWidthWithMargin, tooltipHeight);

                if (rect.getBottom() > tooltipOverlayBounds.getBottom())
                {
                    arrowTipPos = getDownArrowTipPosWithinTooltipOverlay();
                    tooltipX = (arrowTipPos.x - (textWidthWithMargin * 0.5f));
                    tooltipY = arrowTipPos.y;
                    rect = juce::Rectangle<int>(tooltipX, tooltipY - tooltipHeight - (margin * 2), textWidthWithMargin, tooltipHeight);
                    tooltipIsPositionedBelowAnchor = false;
                }
                else
                {
                    tooltipIsPositionedBelowAnchor = true;
                }

                if (rect.getRight() + margin > tooltipOverlayBounds.getRight())
                {
                    rect.setX(tooltipOverlayBounds.getRight() - (rect.getWidth() + margin));
                }
                else if (rect.getX() < tooltipOverlayBounds.getX())
                {
                    rect.setX(tooltipOverlayBounds.getX());
                }

                rect.expand(shadowMargin, shadowMargin);

                setBounds(rect);
            }

        private:
            juce::Point<int> getDownArrowTipPosWithinTooltipOverlay()
            {
                if (anchor == nullptr || tooltipOverlay == nullptr)
                {
                    return {};
                }

                auto anchorTopCenter = anchor->getBounds().getTopLeft() + juce::Point<int>(anchor->getWidth() / 2, 0);
                return tooltipOverlay->getLocalPoint(anchor->getParentComponent(), anchorTopCenter).translated(0, 2.f * getScale());
            }

            juce::Point<int> getUpArrowTipPosWithinTooltipOverlay()
            {
                if (anchor == nullptr || tooltipOverlay == nullptr)
                {
                    return {};
                }

                auto anchorBottomCenter = anchor->getBounds().getBottomLeft() + juce::Point<int>(anchor->getWidth() / 2, 0);
                return tooltipOverlay->getLocalPoint(anchor->getParentComponent(), anchorBottomCenter).translated(0, 2.f * getScale());
            }

            juce::Point<int> getDownArrowTipPosWithinSelf()
            {
                if (anchor == nullptr)
                {
                    return {};
                }

                auto anchorTopCenter = anchor->getBounds().getTopLeft() + juce::Point<int>(anchor->getWidth() / 2, 0);
                return getLocalPoint(anchor->getParentComponent(), anchorTopCenter);
            }

            juce::Point<int> getUpArrowTipPosWithinSelf()
            {
                if (anchor == nullptr)
                {
                    return {};
                }

                auto anchorBottomCenter = anchor->getBounds().getBottomLeft() + juce::Point<int>(anchor->getWidth() / 2, 0);
                return getLocalPoint(anchor->getParentComponent(), anchorBottomCenter);
            }

            float getTextWidth()
            {
                return getFont().getStringWidth(tooltipText);
            }

            juce::Font getFont()
            {
                auto result = getNimbusSansScaled();
                return result.withHeight(result.getHeight() * 1.5f);
            }

            float getArrowHeightScaled()
            {
                return arrowHeightAtScale1 * getScale();
            }

            const std::function<float()> &getScale;
            const std::function<juce::Font&()> &getNimbusSansScaled;
            std::string tooltipText;
            const juce::Component *anchor = nullptr;
            const juce::Component *tooltipOverlay;

            const float arrowHeightAtScale1 = 2.5f;

            const int shadowMargin = 40;

            melatonin::DropShadow shadow;

            bool tooltipIsPositionedBelowAnchor = true;
    };
} // namespace vmpc_juce::gui::vector
