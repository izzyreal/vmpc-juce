#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "SvgComponent.hpp"
#include "TooltipOverlay.hpp"
#include "InfoTooltip.hpp"

namespace vmpc_juce::gui::vector {

    class Menu : public juce::Component, juce::ComponentListener, juce::FocusChangeListener {

        public:
            Menu(const std::function<float()> &getScaleToUse,
                    const std::function<void()> &showAudioSettingsDialogToUse,
                    const std::function<void()> &resetWindowSizeToUse,
                    const std::function<void()> &openKeyboardScreenToUse,
                    const std::function<void(const bool)> &setKeyboardShortcutTooltipsVisibilityToUse,
                    TooltipOverlay *tooltipOverlayToUse,
                    const std::function<juce::Font&()> &getNimbusSansScaledToUse)
                : getScale(getScaleToUse),
                showAudioSettingsDialog(showAudioSettingsDialogToUse),
                openKeyboardScreen(openKeyboardScreenToUse),
                setKeyboardShortcutTooltipsVisibility(setKeyboardShortcutTooltipsVisibilityToUse),
                resetWindowSize(resetWindowSizeToUse),
                tooltipOverlay(tooltipOverlayToUse),
                getNimbusSansScaled(getNimbusSansScaledToUse)
        {
            juce::Desktop::getInstance().addFocusChangeListener(this);

            menuIcon = new SvgComponent({"bars_3.svg"}, this, 0.f, getScale);
            menuIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(menuIcon);

            if (juce::JUCEApplicationBase::isStandaloneApp())
            {
                speakerIcon = new SvgComponent({"speaker_wave.svg"}, this, 0.f, getScale);
                speakerIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(speakerIcon);
                
#ifndef JUCE_IOS
                resetZoomIcon = new SvgComponent({"arrows_pointing_in.svg"}, this, 0.f, getScale);
                resetZoomIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(resetZoomIcon);
#endif
            }
            else
            {
                resetZoomIcon = new SvgComponent({"arrows_pointing_in.svg"}, this, 0.f, getScale);
                resetZoomIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(resetZoomIcon);
            }

#if JUCE_IOS
            importIcon = new SvgComponent({"arrow_down_on_square.svg"}, this, 0.f, getScale);
            importIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(importIcon);

            exportIcon = new SvgComponent({"arrow_up_on_square.svg"}, this, 0.f, getScale);
            exportIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(exportIcon);
#endif
            helpIcon = new SvgComponent({"question_mark_circle.svg"}, this, 0.f, getScale);
            helpIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(helpIcon);

            keyboardIcon = new SvgComponent({"keyboard_icon.svg"}, this, 0.f, getScale);
            keyboardIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(keyboardIcon);

            setWantsKeyboardFocus(false);

            infoTooltip = new InfoTooltip(getScale, getNimbusSansScaled, tooltipOverlay);
            tooltipOverlay->addChildComponent(infoTooltip);
        }

            void globalFocusChanged(juce::Component *c) override
            {
                if (c != nullptr)
                {
                    return;
                }

                setKeyboardShortcutTooltipsVisibility(false);
                infoTooltip->setVisible(false);
            }

            void mouseMove(const juce::MouseEvent &e) override
            {
                if (e.getPosition() == lastKnownMousePos)
                {
                    return;
                }

                lastKnownMousePos = e.getPosition();

                if (!getIconBounds().contains(e.getPosition()))
                {
                    mouseExit(e);
                    return;
                }

                const auto iconAtPosition = getIconAtPosition(e.getPosition());

                addTooltip(iconAtPosition);

                if (iconAtPosition == menuIcon && !dontExpandUponMove)
                {
                    mouseOverExpansion = true;

                    for (auto &icon : getPlatformAvailableIcons())
                    {
                        if (icon == menuIcon) continue;
                        icon->setVisible(true);
                    }

                    resized();
                    repaint();
                    return;
                }

                for (auto &icon : getPlatformAvailableIcons())
                {
                    if (icon == menuIcon)
                    {
                        continue;
                    }

                    icon->setAlpha(icon == iconAtPosition ?  0.5f : 1.f);
                }

                if (expanded || mouseOverExpansion)
                {
                    setKeyboardShortcutTooltipsVisibility(iconAtPosition == keyboardIcon);
                }
            }

            void mouseExit(const juce::MouseEvent &e) override
            {
                setKeyboardShortcutTooltipsVisibility(false);
                infoTooltip->setVisible(false);
                mouseOverExpansion = false;
                dontExpandUponMove = false;

                for (auto &icon : getPlatformAvailableIcons())
                {
                    if (icon == menuIcon) continue;
                    icon->setVisible(false || expanded);
                    icon->setAlpha(1.f);
                }

                resized();
                repaint();
            }

            void mouseDown(const juce::MouseEvent &e) override
            {
                if (menuIcon->getBounds().contains(e.getPosition()))
                {
                    expanded = !expanded;
                    dontExpandUponMove = !expanded;
                    menuIcon->setAlpha(expanded ? 1.f : 0.5f);

                    if (!expanded)
                    {
                        mouseOverExpansion = false;
                    }

                    for (auto &icon : getPlatformAvailableIcons())
                    {
                        if (icon == menuIcon) continue;
                        icon->setVisible(expanded);
                        icon->setAlpha(1.f);
                    }

                    resized();
                    repaint();
                    return;
                }

                if (!expanded && !mouseOverExpansion)
                {
                    return;
                }

                SvgComponent *const clickedIcon = getIconAtPosition(e.getPosition()); 

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
                    openKeyboardScreen();
                }
                else if (clickedIcon == helpIcon)
                {
                    juce::URL url("https://vmpcdocs.izmar.nl");
                    url.launchInDefaultBrowser();
                }
                else if (clickedIcon == resetZoomIcon)
                {
                    resetWindowSize();
                }

                clickedIcon->setAlpha(1.f);
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

                const auto fixedHeight = (heightAtScale1 * scale) - (lineThickness * 3);
                const auto centerY = (getHeight() - fixedHeight) / 2.0f;

                auto rect = getIconBounds().toFloat().expanded(margin, lineThickness);
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
                juce::Desktop::getInstance().removeFocusChangeListener(this);

                tooltipOverlay->removeChildComponent(infoTooltip);

                delete infoTooltip;

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
            void addTooltip(SvgComponent *icon)
            {
                std::string tooltipText;

                if (icon == menuIcon)
                {
                    tooltipText = "Open/close/pin menu";
                }
                else if (icon == speakerIcon)
                {
                    tooltipText = "Audio/MIDI settings";
                }
                else if (icon == resetZoomIcon)
                {
                    tooltipText = "Restore default window size";
                }
                else if (icon == helpIcon)
                {
                    tooltipText = "Browse online documentation";
                }
                else if (icon == keyboardIcon)
                {
                    tooltipText = "Show/configure keyboard";
                }

                if (!tooltipText.empty())
                {
                    infoTooltip->configure(tooltipText, icon);
                    infoTooltip->setVisible(true);
                }
                else
                {
                    infoTooltip->setVisible(false);
                }
            }

            std::vector<SvgComponent*> getPlatformAvailableIcons()
            {
                std::vector<SvgComponent*> result { menuIcon };

                if (juce::JUCEApplication::isStandaloneApp())
                {
                    result.push_back(speakerIcon);
#ifndef JUCE_IOS
                    result.push_back(resetZoomIcon);
#endif
                }
                else
                {
                    result.push_back(resetZoomIcon);
                }
#if JUCE_IO
                result.push_back(importIcon);
                result.push_back(exportIcon);
#endif
                result.push_back(keyboardIcon);
                result.push_back(helpIcon);

                return result;
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

            juce::Rectangle<int> getIconBounds()
            {
                juce::Rectangle<int> iconBounds;

                for (SvgComponent* icon : getPlatformAvailableIcons())
                {
                    if (icon->isVisible())
                        iconBounds = iconBounds.isEmpty() ? icon->getBounds() : iconBounds.getUnion(icon->getBounds());
                }

                return iconBounds;
            }

            const std::function<float()> &getScale;
            bool expanded = true;
            bool mouseOverExpansion = false;
            bool dontExpandUponMove = false;
            const std::function<void()> &showAudioSettingsDialog;
            const std::function<void()> resetWindowSize;
            const std::function<void()> openKeyboardScreen;
            const std::function<void(const bool)> setKeyboardShortcutTooltipsVisibility;
            const std::function<juce::Font&()> &getNimbusSansScaled;

            TooltipOverlay *tooltipOverlay;
            InfoTooltip *infoTooltip = nullptr;

            SvgComponent *menuIcon = nullptr;
            SvgComponent *speakerIcon = nullptr;
            SvgComponent *importIcon = nullptr;
            SvgComponent *exportIcon = nullptr;
            SvgComponent *resetZoomIcon = nullptr;
            SvgComponent *helpIcon = nullptr;
            SvgComponent *keyboardIcon = nullptr;

            // mouseMove is triggered also when modifier keys have changed.
            // We only want to know about actual mouse moves, so we keep track of
            // the last known mouse position.
            juce::Point<int> lastKnownMousePos {-1, -1};
    };
} // namespace vmpc_juce::gui::vector 
