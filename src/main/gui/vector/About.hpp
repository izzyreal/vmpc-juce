#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "VmpcJuceResourceUtil.hpp"
#include "juce_graphics/juce_graphics.h"

namespace vmpc_juce::gui::vector {

    const float marginAtScale1 = 8.f;

    class AboutBorder : public juce::Component {
        public:
            AboutBorder(const std::function<float()> &getScaleToUse) : getScale(getScaleToUse) {}
            void paint(juce::Graphics &g)
            {
                const auto scale = getScale();
                const auto lineThickness = scale;
                const auto radius = scale * 3;
                const auto margin = scale * marginAtScale1;
                const auto rect = getLocalBounds().toFloat().reduced(lineThickness * 0.5).withTrimmedLeft(radius).withTrimmedRight(radius);
                const auto top = rect.withTrimmedBottom(getHeight() - margin);
                const auto bottom = rect.withTrimmedTop(getHeight() - margin);

                g.setColour(juce::Colours::white);
                g.fillRect(top); g.fillRect(bottom);

                const auto rect2 = getLocalBounds().toFloat().reduced(lineThickness * 0.5);

                g.setColour(juce::Colours::black);
                g.drawRoundedRectangle(rect2, radius, lineThickness);
            }

        private:
            const std::function<float()> &getScale;
    };

    class About : public juce::Component {
        public:
            About(const std::function<float()> &getScaleToUse, const std::function<juce::Font&()> &getNimbusSansScaledToUse)
                : getScale(getScaleToUse), getNimbusSansScaled(getNimbusSansScaledToUse)
            {
                aboutBorder = new AboutBorder(getScale);
                const auto creditsTextData = vmpc_juce::VmpcJuceResourceUtil::getResourceData("txt/credits.txt");
                creditsText = std::string(creditsTextData.begin(), creditsTextData.end());
                label.setColour(juce::Label::ColourIds::textColourId, juce::Colours::black);
                label.setText(creditsText, juce::dontSendNotification);
                addAndMakeVisible(label);
                addAndMakeVisible(aboutBorder);
            }

            void paint(juce::Graphics &g) override
            {
                const auto rect = getLocalBounds().toFloat().reduced(getScale());

                g.setColour(juce::Colours::white);
                g.fillRoundedRectangle(rect, getScale());
            }

            void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w) override
            {
                const auto maxScrollOffset = (float) label.getBounds().getHeight() - getHeight();

                const auto oldScrollOffset = scrollOffset;

                scrollOffset = std::clamp<float>(scrollOffset - (50 * w.deltaY), 0, maxScrollOffset);

                if (scrollOffset == oldScrollOffset)
                {
                    return;
                }

                resized();
                repaint();
            }

            void resized() override
            {
                aboutBorder->setBounds(0, 0, getWidth(), getHeight());

                const auto margin = marginAtScale1 * getScale();

                if (previousHeight != 0 && getHeight() != previousHeight)
                {
                    const float resizeFactor = getHeight() / float(previousHeight);
                    scrollOffset *= resizeFactor;
                }

                auto f = getNimbusSansScaled();
                f.setHeight(f.getHeight() * 1.5);
                label.setFont(f);
                label.setJustificationType(juce::Justification::top);
                label.setBounds(margin, margin - scrollOffset, getWidth() - std::ceil(margin * 2.f), 86 * f.getHeight());

                previousHeight = getHeight();
            }

            ~About() override
            {
                delete aboutBorder;
            }

        private:
            const std::function<float()> &getScale;
            const std::function<juce::Font&()> &getNimbusSansScaled;
            juce::Label label;
            std::string creditsText;
            double scrollOffset = 0.f;
            int previousHeight = 0.f;
            juce::Component *aboutBorder = nullptr;
    };
} // namespace vmpc_juce::gui::vector
