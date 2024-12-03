#pragma once

#include "juce_graphics/juce_graphics.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector {

    class Tooltip : public juce::Component, juce::ComponentListener {
        public:
            Tooltip(const std::function<std::string()> &getTooltipTextToUse, juce::Component *const positionAnchorToUse, const std::function<juce::Font&()> &getFontToUse)
                : getTooltipText(getTooltipTextToUse), positionAnchor(positionAnchorToUse), getFont(getFontToUse)
            {
                setSize(200, 23);
                positionAnchor->getParentComponent()->addComponentListener(this);
            }

            ~Tooltip() override
            {
                positionAnchor->getParentComponent()->removeComponentListener(this);
            }

            void componentMovedOrResized(juce::Component &, bool wasMoved, bool wasResized) override
            {
                auto targetBounds = positionAnchor->getLocalBounds();
                auto targetCenter = positionAnchor->localPointToGlobal(targetBounds.getCentre());

                auto tooltipParent = getParentComponent();
                if (tooltipParent != nullptr)
                    targetCenter = tooltipParent->getLocalPoint(nullptr, targetCenter);

                int tooltipWidth = getWidth();
                int tooltipHeight = getHeight();
                int newX = targetCenter.x - (tooltipWidth / 2);
                int newY = targetCenter.y - (tooltipHeight / 2);

                setBounds(newX, newY, tooltipWidth, tooltipHeight);
            }

            void paint(juce::Graphics &g) override
            {
                const auto horizontalMarginBetweenTextAndBorder = 4.f;
                const auto bidirectionalMarginBetweenTextAndBorder = 5.f;
                g.setFont(getFont());
                g.setFont(getHeight() - (bidirectionalMarginBetweenTextAndBorder * 2));

                std::string tooltipText = getTooltipText();
                for (auto &c : tooltipText) c = toupper(c);

                const auto textWidth = g.getCurrentFont().getStringWidthFloat(tooltipText);
                const float lineThickness = 2.f;
                const float radius = 3.f;

                const auto rect1 = getLocalBounds().toFloat()
                    .reduced(lineThickness / 2.f)
                    .reduced((getWidth() - textWidth) / 2.f, 0.f)
                    .expanded(lineThickness + horizontalMarginBetweenTextAndBorder, 0.f);
                
                const auto rect2 = rect1.reduced(lineThickness / 2.f);

                g.setColour(juce::Colours::white);
                g.drawRoundedRectangle(rect1, radius, lineThickness);
                g.fillRoundedRectangle(rect2, radius);

                g.setColour(juce::Colours::black);
                g.drawRoundedRectangle(rect2, radius, lineThickness);
                g.drawText(tooltipText, getLocalBounds().reduced(bidirectionalMarginBetweenTextAndBorder), juce::Justification::centred);
            }

        private:
            const std::function<std::string()> getTooltipText;
            juce::Component *const positionAnchor;
            const std::function<juce::Font&()> &getFont;
    };

} // namespace vmpc_juce::gui::vector
