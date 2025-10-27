#include <juce_gui_basics/juce_gui_basics.h>

#include "Constants.hpp"

namespace vmpc_juce::gui::vector
{

    class JOrLShape : public juce::Component
    {

    public:
        enum Shape
        {
            J,
            L
        };

        JOrLShape(const Shape shapeToUse,
                  const std::function<float()> &getScaleToUse)
            : shape(shapeToUse), getScale(getScaleToUse)
        {
        }

        void paint(juce::Graphics &g) override
        {
            g.setColour(Constants::darkLabelColour);

            const auto horizontal_line_thickness =
                Constants::lineThickness1 * getScale();
            const auto vertical_line_thickness =
                Constants::lineThickness2 * getScale();

            const auto half_thickness1 = Constants::lineThickness1 / 2;
            const auto half_thickness2 = Constants::lineThickness2 / 2;

            g.drawLine(0, getHeight() - std::ceil(half_thickness1), getWidth(),
                       getHeight() - std::ceil(half_thickness1),
                       horizontal_line_thickness);

            switch (shape)
            {
                case Shape::J:
                    g.drawLine(getWidth() - half_thickness2, 0,
                               getWidth() - half_thickness2,
                               getHeight() - half_thickness1,
                               vertical_line_thickness);
                    break;
                case Shape::L:
                default:
                    g.drawLine(0 + half_thickness2, 0, 0 + half_thickness2,
                               getHeight() - half_thickness1,
                               vertical_line_thickness);
                    break;
            }
        }

    private:
        const Shape shape;
        const std::function<float()> &getScale;
    };

} // namespace vmpc_juce::gui::vector
