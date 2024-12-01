#pragma once
#include "SvgComponent.hpp"

namespace vmpc_juce::gui::vector {

    class SliderCap : public SvgComponent {
        public:
            SliderCap(juce::Component *commonParentWithShadow, const float shadowSize, const std::function<float()> &getScaleToUse)
                : SvgComponent("slider_cap.svg", commonParentWithShadow, shadowSize, getScaleToUse), getScale(getScaleToUse)
            {
            }

            void paint(juce::Graphics &g) override
            {
                const auto scale = getScale();
                const auto bounds = svgDrawable->getBounds().toFloat().transformedBy(juce::AffineTransform().scaled(scale));
                float height = bounds.getHeight();
                float topPartHeight = height * 0.8f;

                auto topBounds = bounds.withHeight(topPartHeight);
                auto bottomBounds = bounds.withTrimmedTop(topPartHeight);

                g.saveState();
                g.reduceClipRegion(topBounds.toNearestInt());
                svgDrawable->draw(g, 1.f, juce::AffineTransform().scaled(scale));

                g.restoreState();
                g.reduceClipRegion(bottomBounds.toNearestInt());

                const auto verticalScaleFactor = 0.2f + ((1.f - factor) * 0.8f);

                svgDrawable->draw(g, 1.f, juce::AffineTransform().scaled(scale).scaled(1.f, verticalScaleFactor, bounds.getCentreX(), topBounds.getBottom()));
            }

            void setFactor(const float factorToUse)
            {
                factor = factorToUse;
                repaint();
            }

        private:
            const std::function<float()> &getScale;
            float factor = 1.f;
    };

} // namespace vmpc_juce::gui::vector
