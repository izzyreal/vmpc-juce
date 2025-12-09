#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "SvgComponent.hpp"
#include "TooltipOverlay.hpp"
#include "InfoTooltip.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include "gui/ios/ImportDocumentUrlProcessor.hpp"

void doPresentShareOptions(void *nativeWindowHandle, mpc::Mpc *);

void doOpenIosImportDocumentBrowser(
    vmpc_juce::gui::ios::ImportDocumentUrlProcessor *,
    void *nativeWindowHandle);

void doPresentRecordingManager(void *nativeWindowHandle, mpc::Mpc *);

#endif
#endif

namespace vmpc_juce::gui::vector
{

    class Menu : public juce::Component,
                 juce::ComponentListener,
                 juce::FocusChangeListener
    {

    public:
        Menu(
#if TARGET_OS_IPHONE
            mpc::Mpc &mpcToUse,
#endif
            const std::function<float()> &getScaleToUse,
            const std::function<void()> &showAudioSettingsDialogToUse,
            const std::function<void()> &resetWindowSizeToUse,
            const std::function<void()> &openKeyboardScreenToUse,
            const std::function<void(const bool)>
                &setKeyboardShortcutTooltipsVisibilityToUse,
            TooltipOverlay *tooltipOverlayToUse,
            const std::function<juce::Font &()> &getMainFontScaledToUse,
            const std::function<void()> &openAboutToUse,
            juce::AudioProcessor::WrapperType wrapperTypeToUse)
            :
#if TARGET_OS_IPHONE
              mpc(mpcToUse),
#endif
              wrapperType(wrapperTypeToUse), getScale(getScaleToUse),
              showAudioSettingsDialog(showAudioSettingsDialogToUse),
              resetWindowSize(resetWindowSizeToUse),
              openKeyboardScreen(openKeyboardScreenToUse),
              setKeyboardShortcutTooltipsVisibility(
                  setKeyboardShortcutTooltipsVisibilityToUse),
              getMainFontScaled(getMainFontScaledToUse),
              openAbout(openAboutToUse), tooltipOverlay(tooltipOverlayToUse)
        {
            juce::Desktop::getInstance().addFocusChangeListener(this);

            menuIcon = new SvgComponent({"bars_3.svg"}, this, 0.f, getScale);
            menuIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(menuIcon);

            if (juce::JUCEApplicationBase::isStandaloneApp())
            {
                speakerIcon =
                    new SvgComponent({"speaker_wave.svg"}, this, 0.f, getScale);
                speakerIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(speakerIcon);

#if !TARGET_OS_IPHONE
                resetZoomIcon = new SvgComponent({"arrows_pointing_in.svg"},
                                                 this, 0.f, getScale);
                resetZoomIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(resetZoomIcon);
#endif
            }
            else if (wrapperType !=
                     juce::AudioProcessor::wrapperType_AudioUnitv3)
            {
                resetZoomIcon = new SvgComponent({"arrows_pointing_in.svg"},
                                                 this, 0.f, getScale);
                resetZoomIcon->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(resetZoomIcon);
            }

#if TARGET_OS_IPHONE
            importDocumentUrlProcessor.mpc = &mpc;

            importIcon = new SvgComponent({"arrow_down_on_square.svg"}, this,
                                          0.f, getScale);
            importIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(importIcon);

            exportIcon = new SvgComponent({"arrow_up_on_square.svg"}, this, 0.f,
                                          getScale);
            exportIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(exportIcon);

            folderIcon = new SvgComponent({"folder.svg"}, this, 0.f, getScale);
            folderIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(folderIcon);
#endif
            helpIcon = new SvgComponent({"question_mark_circle.svg"}, this, 0.f,
                                        getScale);
            helpIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(helpIcon);

            infoIcon = new SvgComponent({"info_icon.svg"}, this, 0.f, getScale);
            infoIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(infoIcon);

            keyboardIcon =
                new SvgComponent({"keyboard_icon.svg"}, this, 0.f, getScale);
            keyboardIcon->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(keyboardIcon);

            infoTooltip =
                new InfoTooltip(getScale, getMainFontScaled, tooltipOverlay);
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

#if !TARGET_OS_IPHONE
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

            if (iconAtPosition != nullptr)
            {
                addTooltip(iconAtPosition);
            }

            if (iconAtPosition == menuIcon && !dontExpandUponMove)
            {
                mouseOverExpansion = true;

                for (auto &icon : getPlatformAvailableIcons())
                {
                    if (icon == menuIcon)
                    {
                        continue;
                    }
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

                icon->setAlpha(icon == iconAtPosition ? 0.5f : 1.f);
            }

            if (expanded || mouseOverExpansion)
            {
                setKeyboardShortcutTooltipsVisibility(iconAtPosition ==
                                                      keyboardIcon);
            }
        }

        void mouseExit(const juce::MouseEvent &) override
        {
            setKeyboardShortcutTooltipsVisibility(false);
            infoTooltip->setVisible(false);
            mouseOverExpansion = false;
            dontExpandUponMove = false;

            for (auto &icon : getPlatformAvailableIcons())
            {
                if (icon == menuIcon)
                {
                    continue;
                }
                icon->setVisible(false || expanded);
                icon->setAlpha(1.f);
            }

            resized();
            repaint();
        }
#endif

#if TARGET_OS_IPHONE
        void mouseUp(const juce::MouseEvent &e) override
#else
        void mouseUp(const juce::MouseEvent &) override
#endif
        {
            if (speakerIcon != nullptr)
            {
                speakerIcon->setAlpha(1.f);
            }
            if (resetZoomIcon != nullptr)
            {
                resetZoomIcon->setAlpha(1.f);
            }
            if (exportIcon != nullptr)
            {
                exportIcon->setAlpha(1.f);
            }
            if (importIcon != nullptr)
            {
                importIcon->setAlpha(1.f);
            }
            if (folderIcon != nullptr)
            {
                folderIcon->setAlpha(1.f);
            }
            helpIcon->setAlpha(1.f);
            keyboardIcon->setAlpha(1.f);
            infoIcon->setAlpha(1.f);
#if TARGET_OS_IPHONE
            setKeyboardShortcutTooltipsVisibility(false);
            infoTooltip->setVisible(false);
            handleClick(e);
#endif
        }

        void mouseDown(const juce::MouseEvent &e) override
        {
#if TARGET_OS_IPHONE
            const auto icon = getIconAtPosition(e.getPosition());

            if (icon == nullptr || (!expanded && icon != menuIcon))
            {
                return;
            }

            if (icon != menuIcon)
            {
                icon->setAlpha(0.5f);
            }

            if (icon == keyboardIcon)
            {
                setKeyboardShortcutTooltipsVisibility(true);
            }

            addTooltip(icon);
#else
            handleClick(e);
#endif
        }

        void resized() override
        {
            const auto scale = getScale();
            const auto lineThickness = 1.f * scale;

            juce::Grid grid;
            grid.justifyContent = juce::Grid::JustifyContent::end;
            grid.templateRows = {juce::Grid::Fr(1)};

            std::vector<SvgComponent *> visibleIcons{menuIcon};

            auto iconsToDraw = getPlatformAvailableIcons();

            if (expanded || mouseOverExpansion)
            {
                for (size_t i = 1; i < iconsToDraw.size(); i++)
                {
                    visibleIcons.insert(visibleIcons.begin(), iconsToDraw[i]);
                }
            }

            for (int8_t idx = static_cast<int8_t>(visibleIcons.size()) - 1;
                 idx >= 0; idx--)
            {
                const auto icon = visibleIcons[static_cast<size_t>(idx)];
                grid.templateColumns.insert(
                    0, juce::Grid::Px((static_cast<float>(getWidth()) /
                                       float(totalAvailableIconCount)) -
                                      (lineThickness * 2)));

                juce::GridItem::Margin margin{3.f * scale};

                if (icon == menuIcon)
                {
                    margin.right *= 1.5f;
                }
                else if (icon == speakerIcon)
                {
                    margin = {2.2f * scale};
                }
                else if (icon == keyboardIcon)
                {
                    margin.left *= 0.5f;
                    margin.right *= 0.5f;
                }
                else if (icon == helpIcon || icon == infoIcon)
                {
                    margin = {2.5f * scale};
                }
                else if (icon == folderIcon)
                {
                    margin.right = 2.f * scale;
                }
                else if (icon == importIcon || icon == exportIcon)
                {
                    margin.left = 2.5f * scale;
                    margin.right = 2.5f * scale;
                }

                auto item = juce::GridItem(icon)
                                .withArea(1, idx + 1, 1, idx + 1)
                                .withMargin(margin);
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

            const auto fixedHeight =
                (heightAtScale1 * scale) - (lineThickness * 3);
            const auto centerY =
                (static_cast<float>(getHeight()) - fixedHeight) / 2.0f;

            auto rect =
                getIconBounds().toFloat().expanded(margin, lineThickness);
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

        ~Menu() override
        {
            juce::Desktop::getInstance().removeFocusChangeListener(this);

            delete infoTooltip;

            delete menuIcon;
            delete speakerIcon;
            delete importIcon;
            delete exportIcon;
            delete resetZoomIcon;
            delete helpIcon;
            delete keyboardIcon;
            delete folderIcon;
            delete infoIcon;
        }

        const static int totalAvailableIconCount = 9;
        constexpr static const float widthAtScale1 =
            15.f * totalAvailableIconCount * 1.1;
        constexpr static const float heightAtScale1 = 16.f * 1.1f;

    private:
        void handleClick(const juce::MouseEvent e)
        {
            if (menuIcon->getBounds()
                    .expanded(static_cast<int>(getScale() * 3.f))
                    .contains(e.getPosition()))
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
                    if (icon == menuIcon)
                    {
                        continue;
                    }
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

            SvgComponent *const clickedIcon =
                getIconAtPosition(e.getPosition());

            if (clickedIcon == nullptr)
            {
                return;
            }
#if !TARGET_OS_IPHONE
            clickedIcon->setAlpha(0.5f);
#endif
            if (clickedIcon == speakerIcon)
            {
                showAudioSettingsDialog();
            }
#if TARGET_OS_IPHONE
            else if (clickedIcon == importIcon)
            {
                auto uiView = getPeer()->getNativeHandle();
                doOpenIosImportDocumentBrowser(&importDocumentUrlProcessor,
                                               uiView);
            }
            else if (clickedIcon == exportIcon)
            {
                auto uiView = getPeer()->getNativeHandle();
                doPresentShareOptions(uiView, &mpc);
            }
            else if (clickedIcon == folderIcon)
            {
                auto uiView = getPeer()->getNativeHandle();
                doPresentRecordingManager(uiView, &mpc);
            }
#endif
            else if (clickedIcon == keyboardIcon)
            {
                openKeyboardScreen();
            }
            else if (clickedIcon == helpIcon)
            {
                juce::URL url("https://vmpcdocs.izmar.nl");
                url.launchInDefaultBrowser();
            }
            else if (clickedIcon == infoIcon)
            {
                openAbout();
            }
            else if (clickedIcon == resetZoomIcon)
            {
                resetWindowSize();
            }
        }

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
            else if (icon == infoIcon)
            {
                tooltipText = "About";
            }
            else if (icon == keyboardIcon)
            {
                tooltipText = "Show/configure keyboard";
            }
            else if (icon == importIcon)
            {
                tooltipText = "Import";
            }
            else if (icon == exportIcon)
            {
                tooltipText = "Export";
            }
            else if (icon == folderIcon)
            {
                tooltipText = "Recording manager";
            }

            if (infoTooltip->isVisible() && infoTooltip->getAnchor() == icon)
            {
                return;
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

        std::vector<SvgComponent *> getPlatformAvailableIcons()
        {
            std::vector<SvgComponent *> result{menuIcon};

            if (juce::JUCEApplication::isStandaloneApp())
            {
                result.push_back(speakerIcon);
#if !TARGET_OS_IPHONE
                result.push_back(resetZoomIcon);
#endif
            }
            else if (wrapperType !=
                     juce::AudioProcessor::wrapperType_AudioUnitv3)
            {
                result.push_back(resetZoomIcon);
            }
#if TARGET_OS_IPHONE
            result.push_back(importIcon);
            result.push_back(exportIcon);
            result.push_back(folderIcon);
#endif
            result.push_back(keyboardIcon);
            result.push_back(helpIcon);
            result.push_back(infoIcon);
            return result;
        }

        SvgComponent *getIconAtPosition(const juce::Point<int> &position)
        {
            for (auto &icon : getPlatformAvailableIcons())
            {
                if (icon->getBounds()
                        .expanded(static_cast<int>(getScale() * 3.f))
                        .contains(position))
                {
                    return icon;
                }
            }
            return nullptr;
        }

        juce::Rectangle<int> getIconBounds()
        {
            juce::Rectangle<int> iconBounds;

            for (SvgComponent *icon : getPlatformAvailableIcons())
            {
                if (icon->isVisible())
                {
                    iconBounds = iconBounds.isEmpty()
                                     ? icon->getBounds()
                                     : iconBounds.getUnion(icon->getBounds());
                }
            }

            return iconBounds;
        }

#if TARGET_OS_IPHONE
        mpc::Mpc &mpc;
#endif
        juce::AudioProcessor::WrapperType wrapperType;
        const std::function<float()> &getScale;
        bool expanded = true;
        bool mouseOverExpansion = false;
        bool dontExpandUponMove = false;
        const std::function<void()> &showAudioSettingsDialog;
        const std::function<void()> resetWindowSize;
        const std::function<void()> openKeyboardScreen;
        const std::function<void(const bool)>
            setKeyboardShortcutTooltipsVisibility;
        const std::function<juce::Font &()> &getMainFontScaled;
        const std::function<void()> openAbout;

        TooltipOverlay *tooltipOverlay;
        InfoTooltip *infoTooltip = nullptr;

        SvgComponent *menuIcon = nullptr;
        SvgComponent *speakerIcon = nullptr;
        SvgComponent *importIcon = nullptr;
        SvgComponent *exportIcon = nullptr;
        SvgComponent *resetZoomIcon = nullptr;
        SvgComponent *helpIcon = nullptr;
        SvgComponent *keyboardIcon = nullptr;
        SvgComponent *folderIcon = nullptr;
        SvgComponent *infoIcon = nullptr;

        // mouseMove is triggered also when modifier keys have changed.
        // We only want to know about actual mouse moves, so we keep track of
        // the last known mouse position.
        juce::Point<int> lastKnownMousePos{-1, -1};

#if TARGET_OS_IPHONE
        vmpc_juce::gui::ios::ImportDocumentUrlProcessor
            importDocumentUrlProcessor;
#endif
    };
} // namespace vmpc_juce::gui::vector
