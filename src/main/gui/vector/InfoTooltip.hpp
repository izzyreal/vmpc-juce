#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

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

                rect = rect.withTrimmedTop(arrowHeight);

                rect.reduce(lineThickness, lineThickness);

                juce::Path path;
                auto triangleWidth = arrowHeight;
                auto rectTopY = rect.getY();
                auto rectBottomY = rect.getBottom();
                auto rectLeftX = rect.getX();
                auto rectRightX = rect.getRight();
                auto arrowX = getArrowTipPosWithinSelf().x;

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
                path.closeSubPath();

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
                const auto arrowTipPos = getArrowTipPosWithinTooltipOverlay();
                const auto textWidth = getTextWidth();
                const auto textHeight = getFont().getHeight();
                const auto margin = 3.f * scale;
                const auto textWidthWithMargin = textWidth + (margin * 2);
                const auto textHeightWithMargin = textHeight + (margin * 2);
                const auto tooltipX = (arrowTipPos.x - (textWidthWithMargin * 0.5f));
                const auto tooltipY = arrowTipPos.y;
                const auto tooltipHeight = textHeightWithMargin + getArrowHeightScaled();
                auto rect = juce::Rectangle<int>(tooltipX, tooltipY, textWidthWithMargin, tooltipHeight);

                auto tooltipOverlayBounds = tooltipOverlay->getBounds();

                if (rect.getRight() > tooltipOverlayBounds.getRight())
                {
                    rect.setX(tooltipOverlayBounds.getRight() - rect.getWidth());
                }
                else if (rect.getX() < tooltipOverlayBounds.getX())
                {
                    rect.setX(tooltipOverlayBounds.getX());
                }

                setBounds(rect);
            }

        private:
            juce::Point<int> getArrowTipPosWithinTooltipOverlay()
            {
                if (anchor == nullptr || tooltipOverlay == nullptr)
                {
                    return {};
                }

                auto anchorBottomCenter = anchor->getBounds().getBottomLeft() + juce::Point<int>(anchor->getWidth() / 2, 0);
                return tooltipOverlay->getLocalPoint(anchor->getParentComponent(), anchorBottomCenter).translated(0, 2.f * getScale());
            }

            juce::Point<int> getArrowTipPosWithinSelf()
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
    };
} // namespace vmpc_juce::gui::vector
