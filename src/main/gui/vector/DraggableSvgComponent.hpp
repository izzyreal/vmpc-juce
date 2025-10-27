#pragma once

#include "SvgComponent.hpp"
#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector
{

    class DraggableSvgComponent : public SvgComponent
    {
    private:
        juce::ComponentDragger dragger;
        juce::ComponentBoundsConstrainer constrainer;

    public:
        DraggableSvgComponent(const std::vector<std::string> &svg_paths,
                              juce::Component *commonParentWithShadowToUse,
                              const float shadowSizeToUse,
                              const std::function<float()> &getScaleToUse)
            : SvgComponent(svg_paths, commonParentWithShadowToUse,
                           shadowSizeToUse, getScaleToUse)
        {
        }

        void resized() override
        {
            SvgComponent::resized();
            constrainer.setMinimumOnscreenAmounts(
                getParentWidth(), getParentWidth(), getParentHeight(),
                getParentHeight());
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
        }

    protected:
        float getNormalizedY() const
        {
            if (auto *parent = getParentComponent())
            {
                const float range =
                    static_cast<float>(parent->getHeight() - getHeight());

                if (range <= 0.0f)
                {
                    return 0.0f;
                }

                const float normY = static_cast<float>(getY()) / range;
                return std::clamp(normY, 0.0f, 1.0f);
            }

            return 0.0f;
        }
    };
} // namespace vmpc_juce::gui::vector
