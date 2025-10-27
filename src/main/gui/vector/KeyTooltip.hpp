#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <melatonin_blur/melatonin/shadows.h>
#include <melatonin_blur/melatonin_blur.h>

namespace vmpc_juce::gui::vector {

    class KeyTooltip : public juce::Component, juce::ComponentListener {
        public:
            KeyTooltip(
                    const std::function<std::string()> &getTooltipTextToUse,
                    juce::Component *const positionAnchorToUse,
                    const std::pair<float, float> unscaledOffsetFromAnchorToUse,
                    const std::function<juce::Font&()> &getFontToUse,
                    const std::function<float()> &getScaleToUse,
                    const std::string hardwareLabelToUse)
                : getTooltipText(getTooltipTextToUse),
                positionAnchor(positionAnchorToUse),
                getFont(getFontToUse),
                getScale(getScaleToUse),
                hardwareLabel(hardwareLabelToUse),
                unscaledOffsetFromAnchor(unscaledOffsetFromAnchorToUse)
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
                const float scaleFactor = std::log(getScale() + 2.0f) / std::log(2.0f);

                const auto translateX = (unscaledOffsetFromAnchor.first - 2.5f) * scaleFactor;
                const auto translateY = (unscaledOffsetFromAnchor.second + 1.5f) * scaleFactor;

                auto targetBounds = positionAnchor->getLocalBounds().toFloat()
                    .translated(translateX, translateY);
                auto targetCenter = positionAnchor->localPointToGlobal(targetBounds.getCentre());

                auto tooltipParent = getParentComponent();

                if (tooltipParent != nullptr)
                {
                    targetCenter = tooltipParent->getLocalPoint(nullptr, targetCenter);
                }


                const auto tooltipTypeScaleFactor = shouldMimicPhysicalKeyRepresentation() ? 1.1f : 1.f;

                const auto newWidth = 50.f * scaleFactor * tooltipTypeScaleFactor;
                const auto newHeight = 21 * scaleFactor * tooltipTypeScaleFactor;
                const int newX = static_cast<int>(targetCenter.x - newWidth / 2.f);
                const int newY = static_cast<int>(targetCenter.y - newHeight / 2.f);

                setBounds(newX, newY, static_cast<int>(newWidth), static_cast<int>(newHeight));
            }

            void paint(juce::Graphics &g) override
            {
                const float scale = std::log(getScale() + 2.0f) / std::log(2.0f);
                const float lineThickness = 1.5f * scale;
                const auto horizontalMarginBetweenTextAndBorder = 5.f * scale;
                const auto bidirectionalMarginBetweenTextAndBorder = 2.f * scale;
                const auto shadowSize = 4.2f * scale;
                const bool mimicPhysicalKeyRepresentationEnabled = shouldMimicPhysicalKeyRepresentation();
                
                auto fontHeight = (static_cast<float>(getHeight()) * 1.2f - ((bidirectionalMarginBetweenTextAndBorder + lineThickness) * 2)) - (shadowSize * 2);

                if (mimicPhysicalKeyRepresentationEnabled)
                {
                    fontHeight *= 0.8f;
                }

                g.setFont(getFont().withHeight(fontHeight));
                
                const std::string tooltipText = getTooltipText();

                const std::string string1 = mimicPhysicalKeyRepresentationEnabled ? tooltipText.substr(0, 1) : tooltipText;
                const std::string string2 = mimicPhysicalKeyRepresentationEnabled ? tooltipText.substr(1, 1) : "";

                auto textWidth = g.getCurrentFont().getStringWidthFloat(string1);

                const float radius = 3.f;
                const auto totalWidthWithoutText = ((lineThickness + horizontalMarginBetweenTextAndBorder + bidirectionalMarginBetweenTextAndBorder) * 2.f) - (shadowSize * 2);

                const auto totalHeight = (g.getCurrentFont().getHeight() * (mimicPhysicalKeyRepresentationEnabled ? 1.3f : 1.f)) +
                    (lineThickness + bidirectionalMarginBetweenTextAndBorder) * 2;
                
                if (totalWidthWithoutText + textWidth + (shadowSize * 2) > static_cast<float>(getWidth()))
                {
                    textWidth = (static_cast<float>(getWidth()) - totalWidthWithoutText) - (shadowSize * 2);
                    textWidth -= 5.f;
                }

                auto totalWidth = mimicPhysicalKeyRepresentationEnabled ? totalHeight : textWidth + totalWidthWithoutText;

                if (!mimicPhysicalKeyRepresentationEnabled && totalWidth < totalHeight)
                {
                    totalWidth = totalHeight;
                }

                auto outer_rect = juce::Rectangle<float>((static_cast<float>(getWidth()) - (textWidth + totalWidthWithoutText)) / 2, 
                        (static_cast<float>(getHeight()) - totalHeight) / 2, 
                        totalWidth,
                        totalHeight);

                outer_rect = outer_rect.translated(0.f, -(scale * 1.9f));

                auto inner_rect = outer_rect.reduced(lineThickness * 0.5f);

                const auto most_inner_rect = inner_rect.reduced(scale * 0.9f);

                outer_rect = outer_rect.translated(0.f, scale * 1.3f);
                inner_rect = inner_rect.translated(0.f, scale * 1.3f);

                juce::Path shadowPath;
                shadowPath.addRoundedRectangle(inner_rect, radius);

                shadow.setRadius(scale * 2);
                shadow.setOffset(scale * 1.5, scale * 1.5);
                shadow.setSpread(scale * 0.5);
                shadow.render(g, shadowPath);

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
                    g.drawText(string1, outer_rect.translated(0.f, scale * 2.1f), juce::Justification::centred);
                    g.drawText(string2, outer_rect.translated(0.f, -(scale * 4.6f)), juce::Justification::centred);
                }
                else
                {
                    g.drawText(string1, outer_rect.translated(0.f, -(scale * 1.3f)), juce::Justification::centred);
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
            const std::pair<float, float> unscaledOffsetFromAnchor;

            const bool shouldMimicPhysicalKeyRepresentation()
            {
                const auto tooltipText = getTooltipText();
                return tooltipText.length() == 2 && tooltipText.front() != 'F';
            }
    };

} // namespace vmpc_juce::gui::vector
