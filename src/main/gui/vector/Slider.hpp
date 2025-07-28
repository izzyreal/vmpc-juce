#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "RectangleLabel.hpp"
#include "Constants.hpp"
#include "SvgComponent.hpp"
#include "hardware/HwSlider.hpp"
#include "juce_graphics/juce_graphics.h"

namespace vmpc_juce::gui::vector {

class Slider : public juce::Component, juce::Timer {
    public:
        Slider(juce::Component *commonParentWithShadowToUse,
                const std::function<float()> &getScaleToUse,
                const float shadowSize,
                const std::function<juce::Font&()> &getMainFontScaled,
                const std::shared_ptr<mpc::hardware::Slider> mpcSliderToUse)
            : mpcSlider(mpcSliderToUse), getScale(getScaleToUse)
        {
            rectangleLabel = new RectangleLabel(getScaleToUse, "NOTE\nVARIATION", "VARIATION", Constants::chassisColour, Constants::labelColour, 0.f, 7.f, getMainFontScaled);
            addAndMakeVisible(rectangleLabel);

            sliderCapSvg = new SvgComponent({"slider_cap.svg"}, commonParentWithShadowToUse, shadowSize, getScale);
            addAndMakeVisible(sliderCapSvg);
            sliderCapSvg->setInterceptsMouseClicks(false, false);
            startTimer(30);
        }
        
        ~Slider() override
        {
            delete rectangleLabel;
            delete sliderCapSvg;
        }

        void timerCallback() override
        {
            if (mpcSlider->getValue() == lastSyncedMpcSliderValue)
            {
                return;
            }

            const int newCosmeticSliderValue = 127 - mpcSlider->getValue();
            const float newSliderFraction = float(newCosmeticSliderValue) * (1.f / 127.f);
            setSliderYPosFraction(newSliderFraction);
            
            lastSyncedMpcSliderValue = mpcSlider->getValue();
        }

        void handleSliderYPosChanged()
        {
            const auto drawableBounds = sliderCapSvg->getDrawableBounds();
            const auto scale = getScale();
            const auto width = drawableBounds.getWidth() * scale;
            const auto height = drawableBounds.getHeight() * scale;

            const auto sliderStart = getSliderStart();
            const auto sliderEnd   = getSliderEnd();
            const auto sliderYPos  = (sliderEnd - sliderStart) * sliderYPosFraction;

            sliderCapSvg->setBounds((getWidth() - width) / 2, sliderStart + sliderYPos, width, height);
            repaint();
        }
        
        void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &m) override
        {
            sliderYPosFraction += m.deltaY;
            sliderYPosFraction = std::clamp<float>(sliderYPosFraction, 0, 1);
            handleSliderYPosChanged();
        }

        void mouseDown(const juce::MouseEvent &e) override
        {
            const auto pos = e.getMouseDownScreenPosition();

            if (sliderCapSvg->getScreenBounds().contains(pos))
            {
                shouldDragCap = true;
            }
        }

        void mouseUp(const juce::MouseEvent &) override
        {
            previousDragDistanceY = std::numeric_limits<int32_t>::max();
            shouldDragCap = false;
        }

        void mouseDrag(const juce::MouseEvent &e) override
        {
            if (!shouldDragCap)
            {
                return;
            }

            if (previousDragDistanceY == std::numeric_limits<int32_t>::max())
            {
                previousDragDistanceY = 0;
            }
            
            const auto sliderStart = getSliderStart();
            const auto sliderEnd   = getSliderEnd();
            const auto sliderLengthInPixels  = sliderEnd - sliderStart;

            const auto distanceToProcessInPixels = e.getDistanceFromDragStartY() - previousDragDistanceY;
            previousDragDistanceY = e.getDistanceFromDragStartY();
            const auto fractionToAdd = (float) distanceToProcessInPixels / sliderLengthInPixels;
            sliderYPosFraction += fractionToAdd;
            sliderYPosFraction = std::clamp<float>(sliderYPosFraction, 0, 1);
            handleSliderYPosChanged();
        }

        void resized() override
        {
            juce::Grid grid;
            grid.templateRows = { juce::Grid::Fr(1), juce::Grid::Fr(8) };
            grid.templateColumns = { juce::Grid::Fr(1) };

            grid.items.add(juce::GridItem(rectangleLabel).withArea(1, 1, 1, 1));

            grid.performLayout(getLocalBounds());
            handleSliderYPosChanged();
        }

        void paint(juce::Graphics &g) override
        {
            g.setColour(Constants::darkLabelColour);

            const auto scale = getScale();
            const auto outer_rect = getLocalBounds().toFloat().withTop((Constants::BASE_FONT_SIZE + Constants::LINE_SIZE + 1) * scale);

            const auto line_thickness = Constants::lineThickness1 * scale;
            
            g.drawRoundedRectangle(outer_rect.reduced(line_thickness), 0.4 * scale, line_thickness);

            const auto width = getWidth();
            const auto height = getHeight();

            const auto line_length = width * 0.2;
            const auto x_offset = width * 0.18;
            const auto y_interval = height * 0.05;
            const auto y_offset_bottom = height * 0.1;

            for (int i = 0; i < 11; i++)
            {
                const auto y_pos = height - (i * y_interval) - y_offset_bottom;
                g.drawLine(x_offset, y_pos, line_length + x_offset, y_pos, line_thickness);
                g.drawLine(width - line_length - x_offset, y_pos, width - x_offset, y_pos, line_thickness);
            }

            auto notch_rect = getLocalBounds().toFloat().reduced(width * 0.465, height * 0.205);
            notch_rect.setY(height - notch_rect.getHeight() - (y_offset_bottom / 2));

            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(notch_rect, 0.4f * scale, 1.f);
            g.fillRoundedRectangle(notch_rect, 0.4f * scale);
        }

        void setSliderYPosFraction(float newFraction)
        {
            sliderYPosFraction = newFraction;
            handleSliderYPosChanged();
        }

        const float getSliderYPosFraction() { return sliderYPosFraction; }

        const bool isDraggingSliderCap() { return shouldDragCap; }

        SvgComponent *sliderCapSvg = nullptr;

    private:
        float getSliderStart()
        {
            return (float) getHeight() * 0.34f;
        }

        float getSliderEnd()
        {
            return (float) getHeight() * 0.84f;
        }

        const std::shared_ptr<mpc::hardware::Slider> mpcSlider;
        RectangleLabel *rectangleLabel = nullptr;
        const std::function<float()> &getScale;
        float sliderYPosFraction = 0.f;
        int32_t previousDragDistanceY = std::numeric_limits<int32_t>::max();
        bool shouldDragCap = false;
        int lastSyncedMpcSliderValue = -1;
};

} // namespace vmpc_juce::gui::vector
