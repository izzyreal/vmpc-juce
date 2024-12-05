#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "SvgComponent.hpp"
#include "juce_graphics/juce_graphics.h"

namespace vmpc_juce::gui::vector {

    class Menu : public juce::Component {

        public:
            Menu(const std::function<float()> &getScaleToUse)
                : getScale(getScaleToUse)
            {
                menuIcon = new SvgComponent({"bars_3.svg"}, this, 0.f, getScale);
                menuIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(menuIcon);

                speakerIcon = new SvgComponent({"speaker_wave.svg"}, this, 0.f, getScale);
                speakerIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(speakerIcon);

                importIcon = new SvgComponent({"arrow_down_on_square.svg"}, this, 0.f, getScale);
                importIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(importIcon);

                exportIcon = new SvgComponent({"arrow_up_on_square.svg"}, this, 0.f, getScale);
                exportIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(exportIcon);

                resetZoomIcon = new SvgComponent({"arrows_pointing_in.svg"}, this, 0.f, getScale);
                resetZoomIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(resetZoomIcon);

                helpIcon = new SvgComponent({"question_mark_circle.svg"}, this, 0.f, getScale);
                helpIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(helpIcon);

                keyboardIcon = new SvgComponent({"keyboard_icon.svg"}, this, 0.f, getScale);
                keyboardIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(keyboardIcon);
            }

            void mouseDown(const juce::MouseEvent &e) override
            {
                expanded = !expanded;

                if (menuIcon->getBounds().contains(e.getPosition()))
                {
                    menuIcon->setAlpha(menuIcon->getAlpha() > 0.6f ? 0.5f : 1.f);
                }
                else if (speakerIcon->getBounds().contains(e.getPosition()))
                {
                    speakerIcon->setAlpha(speakerIcon->getAlpha() > 0.6f ? 0.5f : 1.f);
                }

                keyboardIcon->setVisible(expanded);
                speakerIcon->setVisible(expanded);
                importIcon->setVisible(expanded);
                exportIcon->setVisible(expanded);
                resetZoomIcon->setVisible(expanded);
                helpIcon->setVisible(expanded);
            }

            void resized() override
            {
                const auto scale = getScale();

                juce::Grid grid;
                grid.justifyContent = juce::Grid::JustifyContent::end;
                grid.templateRows = { juce::Grid::Fr(1) };

                std::vector<SvgComponent*> visibleIcons { menuIcon };

                if (expanded)
                {
                    visibleIcons.insert(visibleIcons.begin(), keyboardIcon);
                    visibleIcons.insert(visibleIcons.begin(), speakerIcon);
                    visibleIcons.insert(visibleIcons.begin(), importIcon);
                    visibleIcons.insert(visibleIcons.begin(), exportIcon);
                    visibleIcons.insert(visibleIcons.begin(), resetZoomIcon);
                    visibleIcons.insert(visibleIcons.begin(), helpIcon);
                }

                for (int8_t idx = visibleIcons.size() - 1; idx >= 0; idx--)
                {
                    const auto icon = visibleIcons[idx];
                    const auto drawableBounds = icon->getDrawableBounds();
                    grid.templateColumns.insert(0, juce::Grid::Px(getWidth()/float(iconCount)));

                    juce::GridItem::Margin margin { 3 * scale };

                    if (icon == menuIcon)
                    {
                        margin.right *= 2.f;
                    }
                    else if (icon == keyboardIcon)
                    {
                        margin.left *= 0.5f; margin.right *= 0.5f;
                    }

                    auto item = juce::GridItem(icon).withArea(1, idx+1, 1, idx+1).withMargin(margin);
                    grid.items.add(item);
                }

                grid.performLayout(getLocalBounds());
            }

            void paint(juce::Graphics &g) override
            {
                const auto scale = getScale();
                const auto radius = 3.f * scale;
                const auto lineThickness = 1.f * scale;
                const auto rect = getLocalBounds().toFloat().reduced(lineThickness);
                g.setColour(juce::Colours::white);
                g.fillRoundedRectangle(rect, radius);
                g.setColour(juce::Colours::black);
                g.drawRoundedRectangle(rect, radius, lineThickness);
            }

            ~Menu()
            {
                delete menuIcon;
                delete speakerIcon;
                delete importIcon;
                delete exportIcon;
                delete resetZoomIcon;
                delete helpIcon;
                delete keyboardIcon;
            }

            const static int iconCount = 7;
            constexpr static const float widthAtScale1 = 18.f * iconCount;
            constexpr static const float heightAtScale1 = 20.f;

        private:
            const std::function<float()> &getScale;
            bool expanded = true;

            SvgComponent *menuIcon = nullptr;
            SvgComponent *speakerIcon = nullptr;
            SvgComponent *importIcon = nullptr;
            SvgComponent *exportIcon = nullptr;
            SvgComponent *resetZoomIcon = nullptr;
            SvgComponent *helpIcon = nullptr;
            SvgComponent *keyboardIcon = nullptr;
    };
} // namespace vmpc_juce::gui::vector 
