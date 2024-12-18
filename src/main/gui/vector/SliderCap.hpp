#pragma once
#include "SvgComponent.hpp"

namespace vmpc_juce::gui::vector {

    class SliderCap : public SvgComponent {
        public:
            SliderCap(juce::Component *commonParentWithShadow, const float shadowSize, const std::function<float()> &getScaleToUse)
                : SvgComponent({"slider_cap.svg"}, commonParentWithShadow, shadowSize, getScaleToUse), getScale(getScaleToUse)
            {
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
