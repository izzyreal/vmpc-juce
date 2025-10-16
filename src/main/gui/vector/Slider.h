#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "Constants.hpp"
#include "gui/vector/SliderCap.h"
#include "juce_graphics/juce_graphics.h"

namespace vmpc_juce::gui::vector {

class Slider: public juce::Component {
    public:
        Slider(mpc::Mpc &mpc, std::shared_ptr<mpc::hardware2::Slider> mpcSlider, const std::function<float()> &getScaleToUse)
            : getScale(getScaleToUse)
        {
            sliderCap = new SliderCap(mpc, mpcSlider, {"slider_cap.svg"}, this, 5, getScale);
            addAndMakeVisible(sliderCap);
        }

        ~Slider() override
        {
            delete sliderCap;
        }
        
        void resized() override
        {
            const auto drawableBounds = sliderCap->getDrawableBounds();
            const auto scale = getScale();
            const auto width = drawableBounds.getWidth() * scale;
            const auto height = drawableBounds.getHeight() * scale;
            sliderCap->setSize(width, height);
            sliderCap->setTopLeftPosition(getLocalBounds().getCentreX() - (width *0.5f), 50);
        }

        void paint(juce::Graphics &g) override
        {
            auto transform = juce::AffineTransform::scale(0.975f, 0.975f, (float) getWidth() * 0.5f, (float) getHeight() * 0.5f);
            g.addTransform(transform);

            g.setColour(Constants::darkLabelColour);

            const auto scale = getScale();
            const auto line_thickness = Constants::lineThickness1 * scale;
            
            const auto width = getWidth();
            const auto height = getHeight();

            const auto lineCount = 11;

            const auto line_length = (float) width * 0.35f;
            const auto y_interval = (float) height / (lineCount + 1.f);

            for (int i = 0; i < lineCount; i++)
            {
                const auto y_pos = ((float) i + 1.f) * y_interval;
                g.drawLine(0, y_pos, line_length, y_pos, line_thickness);
                g.drawLine((float) width - line_length, y_pos, (float) width, y_pos, line_thickness);
            }

            auto notch_rect = getLocalBounds().toFloat().reduced((float)width * 0.465f, 2.f);

            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(notch_rect, 0.4f * scale, 1.f);
            g.fillRoundedRectangle(notch_rect, 0.4f * scale);
        }

    private:
        const std::function<float()> &getScale;
        SliderCap *sliderCap;
        juce::ComponentDragger sliderCapDragger;
};

} // namespace vmpc_juce::gui::vector

