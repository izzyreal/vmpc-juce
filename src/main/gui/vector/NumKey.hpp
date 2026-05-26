#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "SimpleLabel.hpp"
#include "Constants.hpp"
#include "SvgComponent.hpp"
#include "KeyComponent.hpp"
#include "RectangleLabel.hpp"

namespace vmpc_juce::gui::vector
{

    class NumKey : public juce::Component
    {
    public:
        NumKey(const std::function<float()> &getScaleToUse,
               const std::string topLabelToUse,
               const std::string bottomLabelToUse, std::string svgPath,
               std::string keyHoleSvgPath, std::string keyButtonSvgPath,
               juce::Component *commonParentWithShadow, const float shadowSize,
               const std::function<juce::Font &()> &getMainFontScaled)
        {
            topLabel =
                new SimpleLabel(getScaleToUse, topLabelToUse,
                                Constants::darkLabelColour, getMainFontScaled);
            bottomLabel = new RectangleLabel(
                getScaleToUse, bottomLabelToUse, bottomLabelToUse,
                Constants::greyFacePaintColour, Constants::darkLabelColour,
                0.5f, 5.f, getMainFontScaled, 1.f);
            if (!keyHoleSvgPath.empty() && !keyButtonSvgPath.empty())
            {
                svgComponent = new KeyComponent(
                    keyHoleSvgPath, keyButtonSvgPath, commonParentWithShadow,
                    shadowSize, getScaleToUse);
                shadowSvgComponent = dynamic_cast<SvgComponent *>(svgComponent);
            }
            else
            {
                shadowSvgComponent = new SvgComponent(
                    {svgPath}, commonParentWithShadow, shadowSize,
                    getScaleToUse);
                svgComponent = shadowSvgComponent;
            }

            addAndMakeVisible(topLabel);
            addAndMakeVisible(bottomLabel);
            addAndMakeVisible(svgComponent);
        }

        ~NumKey() override
        {
            delete topLabel;
            delete bottomLabel;
            delete svgComponent;
        }

        void resized() override
        {
            juce::Grid grid;
            grid.templateRows = {juce::Grid::Fr(10), juce::Grid::Fr(10),
                                 juce::Grid::Fr(13)};
            grid.templateColumns = {juce::Grid::Fr(1)};

            grid.items.add(juce::GridItem(topLabel).withArea(1, 1, 1, 1));
            grid.items.add(juce::GridItem(svgComponent).withArea(2, 1, 2, 1));
            grid.items.add(juce::GridItem(bottomLabel).withArea(3, 1, 3, 1));

            grid.performLayout(getLocalBounds());
        }

        SvgComponent *getSvgComponent()
        {
            return shadowSvgComponent;
        }

    private:
        SimpleLabel *topLabel = nullptr;
        RectangleLabel *bottomLabel = nullptr;
        juce::Component *svgComponent = nullptr;
        SvgComponent *shadowSvgComponent = nullptr;
    };

} // namespace vmpc_juce::gui::vector
