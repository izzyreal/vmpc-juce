#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "RectangleLabel.hpp"
#include "Constants.hpp"

namespace vmpc_juce::gui::vector
{

    class SliderBorder : public juce::Component
    {
    public:
        SliderBorder(const std::function<float()> &getScaleToUse,
                     const std::function<juce::Font &()> &getMainFontScaled)
            : getScale(getScaleToUse)
        {
            rectangleLabel = new RectangleLabel(
                getScaleToUse, "NOTE\nVARIATION", "VARIATION",
                Constants::chassisColour, Constants::labelColour, 0.f, 7.f,
                getMainFontScaled);
            addAndMakeVisible(rectangleLabel);
        }

        ~SliderBorder() override
        {
            delete rectangleLabel;
        }

        void resized() override
        {
            juce::Grid grid;
            grid.templateRows = {juce::Grid::Fr(1), juce::Grid::Fr(8)};
            grid.templateColumns = {juce::Grid::Fr(1)};
            grid.items.add(juce::GridItem(rectangleLabel).withArea(1, 1, 1, 1));
            grid.performLayout(getLocalBounds());
        }

        void paint(juce::Graphics &g) override
        {
            g.setColour(Constants::darkLabelColour);

            const auto scale = getScale();
            const auto outer_rect = getLocalBounds().toFloat().withTop(
                (Constants::BASE_FONT_SIZE + Constants::LINE_SIZE + 1) * scale);

            const auto line_thickness = Constants::lineThickness1 * scale;

            g.drawRoundedRectangle(outer_rect.reduced(line_thickness),
                                   0.4f * scale, line_thickness);
        }

    private:
        RectangleLabel *rectangleLabel = nullptr;
        const std::function<float()> &getScale;
    };

} // namespace vmpc_juce::gui::vector
