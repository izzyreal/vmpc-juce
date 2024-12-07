#pragma once

#include "juce_graphics/juce_graphics.h"
#include "melatonin_blur/melatonin/shadows.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <melatonin_blur/melatonin_blur.h>

namespace vmpc_juce::gui::vector {

    class KeyTooltip : public juce::Component, juce::ComponentListener {
        public:
            KeyTooltip(const std::function<std::string()> &getTooltipTextToUse, juce::Component *const positionAnchorToUse, const std::function<juce::Font&()> &getFontToUse, const std::function<float()> &getScaleToUse, const std::string hardwareLabelToUse)
                : getTooltipText(getTooltipTextToUse), positionAnchor(positionAnchorToUse), getFont(getFontToUse), getScale(getScaleToUse), hardwareLabel(hardwareLabelToUse)
            {
                shadow.setColor(juce::Colours::black.withAlpha(0.5f));
                positionAnchorParent = positionAnchor->getParentComponent();
                positionAnchorParent->addComponentListener(this);
                const auto newWidth = 100.f * (getScale()*0.5);
                const auto newHeight = 18 * getScale();
                setSize(newWidth, newHeight);
            }

            ~KeyTooltip() override
            {
                positionAnchorParent->removeComponentListener(this);
            }

            void componentMovedOrResized(juce::Component &, bool wasMoved, bool wasResized) override
            {
                auto targetBounds = positionAnchor->getLocalBounds();
                auto targetCenter = positionAnchor->localPointToGlobal(targetBounds.getCentre());

                auto tooltipParent = getParentComponent();
                if (tooltipParent != nullptr)
                    targetCenter = tooltipParent->getLocalPoint(nullptr, targetCenter);
                const float scaleFactor = std::log(getScale() + 2.0f) / std::log(2.0f);
                const auto newWidth = 100.f * scaleFactor;
                const auto newHeight = 21 * scaleFactor;
                int newX = targetCenter.x - (newWidth / 2);
                int newY = targetCenter.y - (newHeight / 2);

                setBounds(newX, newY, newWidth, newHeight);
            }

            void paint(juce::Graphics &g) override
            {
                const float scale = std::log(getScale() + 2.0f) / std::log(2.0f);
                const bool keyboardStyle = getScale() > 1.4f;
                const float lineThickness = keyboardStyle ? 1.5f * scale : 0.7f * scale;
                const auto horizontalMarginBetweenTextAndBorder = 5.f * scale;
                const auto bidirectionalMarginBetweenTextAndBorder = 2.f * scale;
                const auto shadowSize = 6.f + 3.f;
                const auto fontHeight = (getHeight() - ((bidirectionalMarginBetweenTextAndBorder + lineThickness) * 2)) - (shadowSize * 2);

                g.setFont(getFont().withHeight(fontHeight));
                std::string tooltipText = getTooltipText();
                for (auto &c : tooltipText) c = toupper(c);
                auto textWidth = g.getCurrentFont().getStringWidthFloat(tooltipText);

                const float radius = 3.f;
                const auto totalWidthWithoutText = ((lineThickness + horizontalMarginBetweenTextAndBorder + bidirectionalMarginBetweenTextAndBorder) * 2.f) - (shadowSize * 2);
                const auto totalHeight = g.getCurrentFont().getHeight() + (lineThickness + bidirectionalMarginBetweenTextAndBorder) * 2;

                if (totalWidthWithoutText + textWidth + (shadowSize * 2) > getWidth())
                {
                    textWidth = (getWidth() - totalWidthWithoutText) - (shadowSize * 2);
                    textWidth -= 5.f;
                }

                auto outer_rect = juce::Rectangle<float>((getWidth() - (textWidth + totalWidthWithoutText)) / 2, 
                        (getHeight() - totalHeight) / 2, 
                        textWidth + totalWidthWithoutText, 
                        totalHeight);
                auto inner_rect = outer_rect.reduced(lineThickness * 0.5f);

                const auto most_inner_rect = inner_rect.reduced(scale * 0.9f);

                if (keyboardStyle)
                {
                    outer_rect = outer_rect.translated(0.f, scale * 1.3f);
                    inner_rect = inner_rect.translated(0.f, scale * 1.3f);
                }

                juce::Path shadowPath;
                shadowPath.addRoundedRectangle(inner_rect, radius);

                shadow.setRadius(scale * 3);
                shadow.setOffset(scale * 1.5, scale * 1.5);
                shadow.setSpread(scale * 0.5);
                shadow.render(g, shadowPath);

                if (keyboardStyle)
                {
                    g.setColour(juce::Colours::black);
                    g.drawRoundedRectangle(most_inner_rect, radius, lineThickness);
                    g.drawRoundedRectangle(inner_rect, radius, lineThickness);
                    g.setColour(juce::Colours::lightgrey.darker(0.2f));
                    g.fillRoundedRectangle(inner_rect, radius);
                    g.setColour(juce::Colours::white);
                    g.fillRoundedRectangle(most_inner_rect, radius);
                    g.setColour(juce::Colours::black);
                    g.drawText(tooltipText, outer_rect.translated(0.f, -(scale * 1.3f)), juce::Justification::centred);
                }
                else
                {
                    g.setColour(juce::Colours::white);
                    g.fillRoundedRectangle(inner_rect, radius);
                    g.setColour(juce::Colours::black);
                    g.drawRoundedRectangle(inner_rect, radius, lineThickness);
                    g.drawText(tooltipText, outer_rect, juce::Justification::centred);
                }
            }

            const std::string const getHardwareLabel()
            {
                return hardwareLabel;
            }

        private:
            melatonin::DropShadow shadow;
            const std::function<std::string()> getTooltipText;
            juce::Component *const positionAnchor;
            juce::Component *positionAnchorParent;
            const std::function<juce::Font&()> &getFont;
            const std::function<float()> &getScale;
            const std::string hardwareLabel;
    };

} // namespace vmpc_juce::gui::vector
