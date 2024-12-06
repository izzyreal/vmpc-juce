#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "SvgComponent.hpp"
#include "juce_graphics/juce_graphics.h"

namespace vmpc_juce::gui::vector {

    class Menu : public juce::Component, juce::ComponentListener {

        public:
            Menu(const std::function<float()> &getScaleToUse,
                    const std::function<void()> &showAudioSettingsDialogToUse,
                    const std::function<void()> &resetWindowSizeToUse)
                : getScale(getScaleToUse), showAudioSettingsDialog(showAudioSettingsDialogToUse),
                resetWindowSize(resetWindowSizeToUse)
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

            void mouseMove(const juce::MouseEvent &e) override
            {
                if (menuIcon->getBounds().contains(e.getPosition()))
                {
                    mouseOverExpansion = true;

                    for (auto &icon : getPlatformAvailableIcons())
                    {
                        if (icon == menuIcon) continue;
                        icon->setVisible(true);
                    }

                    resized();
                    repaint();
                }

                const auto iconAtPosition = getIconAtPosition(e.getPosition());

                for (auto &icon : getPlatformAvailableIcons())
                {
                    if (icon == menuIcon)
                    {
                        continue;
                    }

                    icon->setAlpha(icon == iconAtPosition ?  0.5f : 1.f);
                }
            }

            void mouseExit(const juce::MouseEvent &e) override
            {
                mouseOverExpansion = false;

                for (auto &icon : getPlatformAvailableIcons())
                {
                    if (icon == menuIcon) continue;
                    icon->setVisible(false || expanded);
                }

                resized();
                repaint();
            }

            void mouseDown(const juce::MouseEvent &e) override
            {
                if (menuIcon->getBounds().contains(e.getPosition()))
                {
                    expanded = !expanded;

                    menuIcon->setAlpha(expanded ? 1.f : 0.5f);
                    return;
                }

                if (!expanded && !mouseOverExpansion)
                {
                    return;
                }

                SvgComponent *clickedIcon = getIconAtPosition(e.getPosition()); 

                if (clickedIcon == nullptr)
                {
                    return;
                }

                if (clickedIcon == speakerIcon)
                {
                    showAudioSettingsDialog();
                }
                else if (clickedIcon == importIcon)
                {
                    printf("import\n");
                }
                else if (clickedIcon == exportIcon)
                {
                    printf("export\n");
                }
                else if (clickedIcon == keyboardIcon)
                {
                    printf("keyboard\n");
                }
                else if (clickedIcon == helpIcon)
                {
                    printf("help\n");
                }
                else if (clickedIcon == resetZoomIcon)
                {
                    resetWindowSize();
                }
            }

            void resized() override
            {
                const auto scale = getScale();
                const auto lineThickness = 1.f * scale;

                juce::Grid grid;
                grid.justifyContent = juce::Grid::JustifyContent::end;
                grid.templateRows = { juce::Grid::Fr(1) };

                std::vector<SvgComponent*> visibleIcons { menuIcon };

                auto iconsToDraw = getPlatformAvailableIcons();

                if (expanded || mouseOverExpansion)
                {
                    for (int i = 1; i < iconsToDraw.size(); i++)
                    {
                        visibleIcons.insert(visibleIcons.begin(), iconsToDraw[i]);
                    }
                }

                for (int8_t idx = visibleIcons.size() - 1; idx >= 0; idx--)
                {
                    const auto icon = visibleIcons[idx];
                    grid.templateColumns.insert(0, juce::Grid::Px((getWidth()/float(totalAvailableIconCount)) - (lineThickness * 2)));

                    juce::GridItem::Margin margin { 3.f * scale };

                    if (icon == menuIcon)
                    {
                        margin.right *= 1.5f;
                    }
                    else if (icon == speakerIcon)
                    {
                        margin = { 2.2f * scale };
                    }
                    else if (icon == keyboardIcon)
                    {
                        margin.left *= 0.5f; margin.right *= 0.5f;
                    }
                    else if (icon == helpIcon)
                    {
                        margin = { 2.5f * scale };
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
                const auto margin = 5.f * scale;

                juce::Rectangle<int> iconBounds;

                for (SvgComponent* icon : getPlatformAvailableIcons())
                {
                    if (icon->isVisible())
                        iconBounds = iconBounds.isEmpty() ? icon->getBounds() : iconBounds.getUnion(icon->getBounds());
                }

                const auto fixedHeight = (heightAtScale1 * scale) - (lineThickness * 3);
                const auto centerY = (getHeight() - fixedHeight) / 2.0f;

                auto rect = iconBounds.toFloat().expanded(margin, lineThickness);
                rect.setY(centerY);
                rect.setHeight(fixedHeight);
                rect = rect.withTrimmedRight(lineThickness * 2);

                if (!expanded && !mouseOverExpansion)
                {
                    rect = rect.withTrimmedLeft(lineThickness * 2);
                }

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

            const static int totalAvailableIconCount = 7;
            constexpr static const float widthAtScale1 = 15.f * totalAvailableIconCount;
            constexpr static const float heightAtScale1 = 16.f;

        private:
            std::vector<SvgComponent*> getPlatformAvailableIcons()
            {
                return {
                    menuIcon, speakerIcon, /* importIcon, exportIcon, */ resetZoomIcon, helpIcon, keyboardIcon
                };
            }

            SvgComponent* getIconAtPosition(const juce::Point<int> &position)
            {
                for (auto &icon : getPlatformAvailableIcons())
                {
                    if (icon->getBounds().contains(position))
                    {
                        return icon;
                    }
                }
                return nullptr;
            }

            const std::function<float()> &getScale;
            bool expanded = true;
            bool mouseOverExpansion = false;
            const std::function<void()> &showAudioSettingsDialog;
            const std::function<void()> resetWindowSize;

            SvgComponent *menuIcon = nullptr;
            SvgComponent *speakerIcon = nullptr;
            SvgComponent *importIcon = nullptr;
            SvgComponent *exportIcon = nullptr;
            SvgComponent *resetZoomIcon = nullptr;
            SvgComponent *helpIcon = nullptr;
            SvgComponent *keyboardIcon = nullptr;
    };
} // namespace vmpc_juce::gui::vector 
