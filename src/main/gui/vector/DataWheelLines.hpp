#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector {

    class DataWheelLines : public juce::Component {
        public:
            DataWheelLines(const std::function<float()> &getScaleToUse)
                : getScale(getScaleToUse)
            {
            }

            void setAngle(const float newAngle)
            {
                angle = newAngle;
            }

            void paint(juce::Graphics &g) override
            {
                const auto width = getWidth();
                const auto height = getHeight();
                const auto radius = static_cast<float>(width) / 2.0f;
                const juce::Point<float> center(width / 2.0f, height / 2.0f);

                const float innerRadius = radius * 0.86f;
                const float lineLength = radius * 0.15f;
                const auto scale = getScale();
                const auto line_thickness = scale * 0.5f;
                const float lineOffset = line_thickness / 2;

                for (int i = 0; i < 12; ++i)
                {
                    if (i == 6) continue;
                    float line_angle = juce::MathConstants<float>::twoPi * i / 12.0f + angle;
                    float brightness = 0.7f + 0.5f * std::cos(line_angle + juce::MathConstants<float>::pi);
                    brightness = std::min<float>(brightness, 0.5f);
                    juce::Colour greyColor = juce::Colours::grey.withBrightness(brightness);
                    juce::Colour blackColor = juce::Colours::black;

                    juce::Point<float> direction(std::cos(line_angle), std::sin(line_angle));
                    juce::Point<float> outerPoint = center + direction * innerRadius;
                    juce::Point<float> innerPoint = outerPoint - direction * lineLength;
                    juce::Point<float> offset(-direction.y * lineOffset, direction.x * lineOffset);

                    g.setColour(blackColor);
                    g.drawLine(innerPoint.x - offset.x, innerPoint.y - offset.y,
                            outerPoint.x - offset.x, outerPoint.y - offset.y, line_thickness);

                    g.setColour(greyColor);
                    g.drawLine(innerPoint.x + offset.x, innerPoint.y + offset.y,
                            outerPoint.x + offset.x, outerPoint.y + offset.y, line_thickness);
                }
            }

        private:
            const std::function<float()> &getScale;
            float angle = 0.f;
    };

} // namespace vmpc_juce::gui::vector
