#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "SimpleLabel.hpp"
#include "Constants.hpp"
#include "SvgComponent.hpp"
#include "RectangleLabel.hpp"

namespace vmpc_juce::gui::vector {

    class NumKey : public juce::Component {
        public:
            NumKey(const std::function<float()> &getScaleToUse, const std::string topLabelToUse, const std::string bottomLabelToUse, std::string svgPath, juce::Component *commonParentWithShadow, const float shadowSize, const std::function<juce::Font&()> &getNimbusSansScaled)
            {
                topLabel = new SimpleLabel(getScaleToUse, topLabelToUse, Constants::darkLabelColour, getNimbusSansScaled);
                bottomLabel = new RectangleLabel(getScaleToUse, bottomLabelToUse, bottomLabelToUse, Constants::greyFacePaintColour, Constants::darkLabelColour, 0.5f, 5.f, getNimbusSansScaled);
                svgComponent = new SvgComponent({svgPath}, commonParentWithShadow, shadowSize, getScaleToUse);

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
                grid.templateRows = { juce::Grid::Fr(10), juce::Grid::Fr(10), juce::Grid::Fr(13) };
                grid.templateColumns = { juce::Grid::Fr(1) };

                grid.items.add(juce::GridItem(topLabel).withArea(1, 1, 1, 1));
                grid.items.add(juce::GridItem(svgComponent).withArea(2, 1, 2, 1));
                grid.items.add(juce::GridItem(bottomLabel).withArea(3, 1, 3, 1));

                grid.performLayout(getLocalBounds());
            }

            SvgComponent* getSvgComponent() { return svgComponent; }

        private:
            SimpleLabel* topLabel = nullptr;
            RectangleLabel* bottomLabel = nullptr;
            SvgComponent* svgComponent = nullptr;
    };

} // namespace vmpc_juce::gui::vector
