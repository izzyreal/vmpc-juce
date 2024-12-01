#pragma once

#include "juce_graphics/juce_graphics.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur/melatonin_blur.h>

#include "ViewUtil.hpp"

namespace vmpc_juce::gui::vector {

class Shadow : public juce::Component {

    public:
        Shadow(const std::function<float()> &getScaleToUse, const std::function<juce::Path()> getPathToUse, const float shadowSizeToUse, const float shadowDarknessToUse, const bool isInnerToUse) :
            getPath(getPathToUse), getScale(getScaleToUse), shadowSize(shadowSizeToUse), shadowDarkness(shadowDarknessToUse), isInner(isInnerToUse)
        {
            setInterceptsMouseClicks(false, false);
        }

        void paint(juce::Graphics &g) override
        {
            auto scale = getScale();
            auto radius = scale * shadowSize;
            juce::Point<float> offset = { 1.f * scale * shadowSize, 0.1f * scale * shadowSize };
            auto spread = 0.f;
            auto color = juce::Colours::black.withAlpha(shadowDarkness);

            auto path = getPath();
            juce::AffineTransform transform;
            const auto shadowDimensions = ViewUtil::getShadowDimensions(shadowSize, scale);

            transform = transform.translated(shadowDimensions);
            path.applyTransform(transform);

            if (isInner)
            {
                melatonin::InnerShadow shadow = { color, radius, offset, spread };
                shadow.render(g, path);
            }
            else
            {
                melatonin::DropShadow shadow = { color, radius, offset, spread };
                shadow.render(g, path);
            }
        }

    private:
        const std::function<juce::Path()> getPath;
        const std::function<float()> &getScale;
        const float shadowSize;
        const float shadowDarkness;
        const bool isInner;
};

} // namespace vmpc_juce::gui::vector
