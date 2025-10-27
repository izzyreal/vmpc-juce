#include <juce_gui_basics/juce_gui_basics.h>

#include "SimpleLabel.hpp"
#include "Constants.hpp"
#include <juce_graphics/juce_graphics.h>

namespace vmpc_juce::gui::vector
{

    class LineFlankedLabel : public juce::Component
    {
    public:
        LineFlankedLabel(const std::string &textToUse,
                         const std::function<float()> &getScaleToUse,
                         const std::function<juce::Font &()> &getMainFontScaled)
            : text(textToUse), getScale(getScaleToUse)
        {
            simpleLabel =
                new SimpleLabel(getScaleToUse, textToUse,
                                Constants::labelColour, getMainFontScaled);
            addAndMakeVisible(simpleLabel);
        }

        ~LineFlankedLabel() override
        {
            delete simpleLabel;
        }

        void resized() override
        {
            juce::FlexBox fb;
            fb.alignItems = juce::FlexBox::AlignItems::center;
            const auto requiredHeight = simpleLabel->getRequiredHeight();
            fb.items.add(juce::FlexItem(*simpleLabel)
                             .withFlex(1.f)
                             .withWidth(simpleLabel->getRequiredWidth())
                             .withHeight(requiredHeight));
            fb.performLayout(getLocalBounds());
        }

        void paint(juce::Graphics &g) override
        {
            // g.fillAll(juce::Colours::yellowgreen);
            const auto textWidth =
                simpleLabel->getRequiredWidth() + (3.f * getScale());
            const auto lineInterruptionStartX = (getWidth() - textWidth) / 2;
            const auto lineInterruptionEndX =
                getWidth() - lineInterruptionStartX;

            const auto line_y_base = getHeight() / 2;
            const auto line_thickness = Constants::lineThickness1 * getScale();

            g.setColour(Constants::darkLabelColour);
            g.drawLine(0, line_y_base, lineInterruptionStartX, line_y_base,
                       line_thickness);
            g.drawLine(lineInterruptionEndX, line_y_base, getWidth(),
                       line_y_base, line_thickness);
        }

    private:
        const std::string text;
        const std::function<float()> &getScale;
        SimpleLabel *simpleLabel = nullptr;
    };

} // namespace vmpc_juce::gui::vector
