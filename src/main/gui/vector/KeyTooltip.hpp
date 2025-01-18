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
                setSize(1, 1);
            }

            ~KeyTooltip() override
            {
                positionAnchorParent->removeComponentListener(this);
            }

            void componentMovedOrResized(juce::Component &, bool /* wasMoved */, bool /* wasResized */) override
            {
                auto targetBounds = positionAnchor->getLocalBounds();
                auto targetCenter = positionAnchor->localPointToGlobal(targetBounds.getCentre());

                auto tooltipParent = getParentComponent();

                if (tooltipParent != nullptr)
                {
                    targetCenter = tooltipParent->getLocalPoint(nullptr, targetCenter);
                }

                const float scaleFactor = std::log(getScale() + 2.0f) / std::log(2.0f);

                const auto tooltipTypeScaleFactor = shouldMimicPhysicalKeyRepresentation() ? 1.1f : 1.f;

                const auto newWidth = 100.f * scaleFactor * tooltipTypeScaleFactor;
                const auto newHeight = 21 * scaleFactor * tooltipTypeScaleFactor;
                const int newX = targetCenter.x - int(newWidth / 2);
                const int newY = targetCenter.y - int(newHeight / 2);

                setBounds(newX, newY, newWidth, newHeight);
            }

            void paint(juce::Graphics &g) override
            {
                const float scale = std::log(getScale() + 2.0f) / std::log(2.0f);
                const bool keyboardStyle = getScale() > 1.4f;
                const float lineThickness = keyboardStyle ? 1.5f * scale : 0.7f * scale;
                const auto horizontalMarginBetweenTextAndBorder = 5.f * scale;
                const auto bidirectionalMarginBetweenTextAndBorder = 2.f * scale;
                const auto shadowSize = 4.2f * scale;
                const bool mimicPhysicalKeyRepresentationEnabled = shouldMimicPhysicalKeyRepresentation();
                
                auto fontHeight = (getHeight() * 1.2f - ((bidirectionalMarginBetweenTextAndBorder + lineThickness) * 2)) - (shadowSize * 2);

                if (mimicPhysicalKeyRepresentationEnabled)
                {
                    fontHeight *= 0.6f;
                }

                g.setFont(getFont().withHeight(fontHeight));
                
                const std::string tooltipText = getTooltipText();

                const std::string string1 = mimicPhysicalKeyRepresentationEnabled ? tooltipText.substr(0, 1) : tooltipText;
                const std::string string2 = mimicPhysicalKeyRepresentationEnabled ? tooltipText.substr(1, 1) : "";

                auto textWidth = g.getCurrentFont().getStringWidthFloat(string1);

                const float radius = 3.f;
                const auto totalWidthWithoutText = ((lineThickness + horizontalMarginBetweenTextAndBorder + bidirectionalMarginBetweenTextAndBorder) * 2.f) - (shadowSize * 2);

                const auto totalHeight = (g.getCurrentFont().getHeight() * (mimicPhysicalKeyRepresentationEnabled ? 1.6f : 1.f)) +
                    (lineThickness + bidirectionalMarginBetweenTextAndBorder) * 2;
                
                if (totalWidthWithoutText + textWidth + (shadowSize * 2) > getWidth())
                {
                    textWidth = (getWidth() - totalWidthWithoutText) - (shadowSize * 2);
                    textWidth -= 5.f;
                }

                auto totalWidth = mimicPhysicalKeyRepresentationEnabled ? totalHeight : textWidth + totalWidthWithoutText;

                if (!mimicPhysicalKeyRepresentationEnabled && totalWidth < totalHeight)
                {
                    totalWidth = totalHeight;
                }

                auto outer_rect = juce::Rectangle<float>((getWidth() - (textWidth + totalWidthWithoutText)) / 2, 
                        (getHeight() - totalHeight) / 2, 
                        totalWidth,
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

                shadow.setRadius(scale * 2);
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

                    if (mimicPhysicalKeyRepresentationEnabled)
                    {
                        g.drawText(string1, outer_rect.translated(0.f, scale * 1.1f), juce::Justification::centred);
                        g.drawText(string2, outer_rect.translated(0.f, -(scale * 4.6f)), juce::Justification::centred);
                    }
                    else
                    {
                        g.drawText(string1, outer_rect.translated(0.f, -(scale * 1.3f)), juce::Justification::centred);
                    }
                }
                else
                {
                    g.setColour(juce::Colours::white);
                    g.fillRoundedRectangle(inner_rect, radius);
                    g.setColour(juce::Colours::black);
                    g.drawRoundedRectangle(inner_rect, radius, lineThickness);
                    g.drawText(string1, outer_rect, juce::Justification::centred);
                }
            }

            const std::string getHardwareLabel()
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

            const bool shouldMimicPhysicalKeyRepresentation()
            {
                const auto tooltipText = getTooltipText();
                return tooltipText.length() == 2 && tooltipText.front() != 'F';
            }
    };

} // namespace vmpc_juce::gui::vector
