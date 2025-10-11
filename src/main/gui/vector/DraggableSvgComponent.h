#pragma once

#include "SvgComponent.hpp"
#include "juce_gui_basics/juce_gui_basics.h"

namespace vmpc_juce::gui::vector {

    class DraggableSvgComponent : public SvgComponent {
        private:
            juce::ComponentDragger dragger;
            juce::ComponentBoundsConstrainer constrainer;

        public:
            DraggableSvgComponent(const std::vector<std::string> &svg_paths,
                                  juce::Component *commonParentWithShadowToUse,
                                  const float shadowSizeToUse,
                                  const std::function<float()> &getScaleToUse)
                : SvgComponent(svg_paths, commonParentWithShadowToUse, shadowSizeToUse, getScaleToUse) {}

            float getNormalizedY() const
            {
                if (auto* parent = getParentComponent())
                {
                    const float range = (float)(parent->getHeight() - getHeight());
                    if (range <= 0.f)
                        return 0.f;

                    return juce::jlimit(0.f, 1.f, (float)getY() / range);
                }
                return 0.f;
            }

            void resized() override
            {
                SvgComponent::resized();
                constrainer.setMinimumOnscreenAmounts(getParentWidth(), getParentWidth(), getParentHeight(), getParentHeight());
            }

            void mouseDown(const juce::MouseEvent &e) override
            {
                dragger.startDraggingComponent(this, e);
            }

            void mouseDrag(const juce::MouseEvent &e) override
            {
                auto xPos = getX();
                dragger.dragComponent(this, e, &constrainer);
                setTopLeftPosition(xPos, getY());

                printf("norm y: %f\n", getNormalizedY());
            }
    };
} // namespace vmpc_juce::gui::vector
