#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur/melatonin_blur.h>

#include "ViewUtil.hpp"

namespace vmpc_juce::gui::vector
{

    class Shadow : public juce::Component
    {

    public:
        Shadow(const std::function<float()> &getScaleToUse,
               const std::function<juce::Path()> getPathToUse,
               const std::function<float()> getShadowSizeMultiplierToUse,
               const std::function<float()> getShadowDarknessMultiplierToUse,
               const float shadowSizeToUse, const float shadowDarknessToUse,
               const bool isInnerToUse)
            : getPath(getPathToUse),
              getShadowSizeMultiplier(getShadowSizeMultiplierToUse),
              getShadowDarknessMultiplier(getShadowDarknessMultiplierToUse),
              getScale(getScaleToUse),
              shadowSize(shadowSizeToUse), shadowDarkness(shadowDarknessToUse),
              isInner(isInnerToUse)
        {
            setInterceptsMouseClicks(false, false);
        }

        void paint(juce::Graphics &g) override
        {
            auto scale = getScale();
            auto radius = scale * shadowSize * getShadowSizeMultiplier();
            const auto effectiveDarkness =
                shadowDarkness * getShadowDarknessMultiplier();
            juce::Point<float> offset = {1.f * scale * shadowSize,
                                         0.1f * scale * shadowSize};
            auto path = getPath();
            juce::AffineTransform transform;
            const auto shadowDimensions =
                ViewUtil::getShadowDimensions(shadowSize, scale);

            transform = transform.translated(shadowDimensions);
            path.applyTransform(transform);

            if (isInner)
            {
                innerShadow.setColor(
                    juce::Colours::black.withAlpha(effectiveDarkness));
                innerShadow.setRadius(radius);
                innerShadow.setOffset(offset.withY(offset.getY() * 4));
                innerShadow.render(g, path);
            }
            else
            {
                dropShadow.setColor(
                    juce::Colours::black.withAlpha(effectiveDarkness));
                dropShadow.setRadius(radius);
                dropShadow.setOffset(offset);
                dropShadow.render(g, path);
            }
        }

    private:
        melatonin::InnerShadow innerShadow;
        melatonin::DropShadow dropShadow;
        const std::function<juce::Path()> getPath;
        const std::function<float()> getShadowSizeMultiplier;
        const std::function<float()> getShadowDarknessMultiplier;
        const std::function<float()> &getScale;
        const float shadowSize;
        const float shadowDarkness;
        const bool isInner;
    };

} // namespace vmpc_juce::gui::vector
