#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::standalone
{
    class DeviceSelectorComponentLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        explicit DeviceSelectorComponentLookAndFeel(
            const juce::Font &mainFontToUse)
            : mainFont(mainFontToUse)
        {
            backgroundColour = juce::Colours::slategrey;
            backgroundColourBright = backgroundColour.brighter(0.2f);
            backgroundColourBrightest = backgroundColour.brighter(0.4f);
            backgroundColourDark = backgroundColour.darker(0.4f);

            textColour = juce::Colours::black;

            setColour(juce::ResizableWindow::backgroundColourId,
                      backgroundColour);

            setColour(juce::Label::textColourId, textColour);

            setColour(juce::ComboBox::backgroundColourId,
                      backgroundColourBrightest);

            setColour(juce::ComboBox::textColourId, textColour);

            setColour(juce::TextButton::buttonColourId, backgroundColourBright);

            setColour(juce::TextButton::textColourOnId, textColour);
            setColour(juce::TextButton::textColourOffId, textColour);
            setColour(juce::ListBox::backgroundColourId,
                      backgroundColourBrightest);
            setColour(juce::ListBox::textColourId, textColour);
            setColour(juce::ListBox::outlineColourId, textColour);
            setColour(juce::ScrollBar::thumbColourId, backgroundColour);
        }

        void
        drawTickBox(juce::Graphics &g, juce::Component &, const float x,
                    const float y, const float w, const float h,
                    const bool ticked, [[maybe_unused]] const bool isEnabled,
                    [[maybe_unused]] const bool shouldDrawButtonAsHighlighted,
                    [[maybe_unused]] const bool shouldDrawButtonAsDown) override
        {
            const juce::Rectangle tickBounds(x, y, w, h);

            g.setColour(textColour);
            g.drawRoundedRectangle(tickBounds, 2.0f, 1.0f);

            if (ticked)
            {
                g.setColour(juce::Colours::black);
                const auto tick = getTickShape(0.75f);
                g.fillPath(tick,
                           tick.getTransformToScaleToFit(
                               tickBounds.reduced(4, 5).toFloat(), false));
            }
        }

        juce::Font getPopupMenuFont() override
        {
            return mainFont.withHeight(18);
        }

        juce::Font getLabelFont(juce::Label &) override
        {
            return mainFont.withHeight(18);
        }
        void drawPopupMenuBackground(juce::Graphics &g, int /*width*/,
                                     int /*height*/) override
        {
            g.fillAll(backgroundColour);
        }

        void drawPopupMenuItem(
            juce::Graphics &g, const juce::Rectangle<int> &area,
            const bool isSeparator, const bool isActive,
            const bool isHighlighted, const bool isTicked,
            const bool hasSubMenu, const juce::String &text,
            const juce::String &shortcutKeyText, const juce::Drawable *icon,
            const juce::Colour *const /*textColourToUse*/) override
        {
            if (isSeparator)
            {
                auto r = area.reduced(5, 0);
                r.removeFromTop(juce::roundToInt(
                    static_cast<float>(r.getHeight()) * 0.5f - 0.5f));

                g.setColour(
                    findColour(juce::PopupMenu::textColourId).withAlpha(0.3f));
                g.fillRect(r.removeFromTop(1));
            }
            else
            {
                auto r = area.reduced(1).translated(0, -2);

                if (isHighlighted && isActive)
                {
                    g.setColour(findColour(
                        juce::PopupMenu::highlightedBackgroundColourId));
                    g.fillRect(r);

                    g.setColour(
                        findColour(juce::PopupMenu::highlightedTextColourId));
                }
                else
                {
                    g.setColour(
                        textColour.withMultipliedAlpha(isActive ? 1.0f : 0.5f));
                }

                r.reduce(juce::jmin(5, area.getWidth() / 20), 0);

                auto font = getPopupMenuFont();

                const auto maxFontHeight =
                    static_cast<float>(r.getHeight()) / 1.2f;

                if (font.getHeight() > maxFontHeight)
                {
                    font.setHeight(maxFontHeight);
                }

                g.setFont(font);

                const auto iconArea =
                    r.removeFromLeft(juce::roundToInt(maxFontHeight)).toFloat();

                if (icon != nullptr)
                {
                    icon->drawWithin(
                        g, iconArea,
                        juce::RectanglePlacement::centred |
                            juce::RectanglePlacement::onlyReduceInSize,
                        1.0f);
                    r.removeFromLeft(juce::roundToInt(maxFontHeight * 0.5f));
                }
                else if (isTicked)
                {
                    const auto tick = getTickShape(1.0f);
                    g.fillPath(tick,
                               tick.getTransformToScaleToFit(
                                   iconArea.reduced(iconArea.getWidth() / 5, 0)
                                       .toFloat(),
                                   true));
                }

                if (hasSubMenu)
                {
                    const auto arrowH = 0.6f * getPopupMenuFont().getAscent();

                    const auto x = static_cast<float>(
                        r.removeFromRight(static_cast<int>(arrowH)).getX());
                    const auto halfH = static_cast<float>(r.getCentreY());

                    juce::Path path;
                    path.startNewSubPath(x, halfH - arrowH * 0.5f);
                    path.lineTo(x + arrowH * 0.6f, halfH);
                    path.lineTo(x, halfH + arrowH * 0.5f);

                    g.strokePath(path, juce::PathStrokeType(2.0f));
                }

                r.removeFromRight(3);
                g.drawFittedText(text, r, juce::Justification::centredLeft, 1);

                if (shortcutKeyText.isNotEmpty())
                {
                    auto f2 = font;
                    f2.setHeight(f2.getHeight() * 0.75f);
                    f2.setHorizontalScale(0.95f);
                    g.setFont(f2);

                    g.drawText(shortcutKeyText, r,
                               juce::Justification::centredRight, true);
                }
            }
        }

        void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                                  const juce::Colour &/*backgroundColour*/,
                                  const bool shouldDrawButtonAsHighlighted,
                                  const bool shouldDrawButtonAsDown) override
        {
            constexpr auto cornerSize = 3.0f;
            const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

            auto baseColour =
                backgroundColour
                    .withMultipliedSaturation(
                        button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                    .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

            if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            {
                baseColour = baseColour.contrasting(
                    shouldDrawButtonAsDown ? 0.2f : 0.05f);
            }

            g.setColour(baseColour);

            const auto flatOnLeft = button.isConnectedOnLeft();
            const auto flatOnRight = button.isConnectedOnRight();
            const auto flatOnTop = button.isConnectedOnTop();
            const auto flatOnBottom = button.isConnectedOnBottom();

            if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
            {
                juce::Path path;
                path.addRoundedRectangle(
                    bounds.getX(), bounds.getY(), bounds.getWidth(),
                    bounds.getHeight(), cornerSize, cornerSize,
                    !(flatOnLeft || flatOnTop), !(flatOnRight || flatOnTop),
                    !(flatOnLeft || flatOnBottom),
                    !(flatOnRight || flatOnBottom));

                g.fillPath(path);

                g.setColour(button.findColour(juce::ComboBox::outlineColourId));
                g.strokePath(path, juce::PathStrokeType(1.0f));
            }
            else
            {
                g.fillRoundedRectangle(bounds, cornerSize);

                g.setColour(button.findColour(juce::ComboBox::outlineColourId));
                g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
            }
        }

        void drawPopupMenuUpDownArrow(juce::Graphics &g, const int width,
                                      const int height,
                                      const bool isScrollUpArrow) override
        {
            g.setGradientFill(juce::ColourGradient(
                backgroundColour, 0.0f, static_cast<float>(height) * 0.5f,
                backgroundColour.withAlpha(0.0f), 0.0f,
                isScrollUpArrow ? static_cast<float>(height) : 0.0f, false));

            g.fillRect(0, 0, width, height);

            const auto hw = static_cast<float>(width) * 0.5f;
            const auto arrowW = static_cast<float>(height) * 0.3f;
            const auto y1 =
                static_cast<float>(height) * (isScrollUpArrow ? 0.6f : 0.3f);
            const auto y2 =
                static_cast<float>(height) * (isScrollUpArrow ? 0.3f : 0.6f);

            juce::Path p;
            p.addTriangle(hw - arrowW, y1, hw + arrowW, y1, hw, y2);

            g.setColour(textColour.withAlpha(0.5f));
            g.fillPath(p);
        }

        juce::Font getTextButtonFont(juce::TextButton &,
                                     const int buttonHeight) override
        {
            return mainFont.withHeight(static_cast<float>(buttonHeight) * 0.7f);
        }

        void drawButtonText(juce::Graphics &g, juce::TextButton &button,
                            bool /*shouldDrawButtonAsHighlighted*/,
                            bool /*shouldDrawButtonAsDown*/) override
        {
            const juce::Font font(
                getTextButtonFont(button, button.getHeight()));

            g.setFont(font);
            g.setColour(
                button
                    .findColour(button.getToggleState()
                                    ? juce::TextButton::textColourOnId
                                    : juce::TextButton::textColourOffId)
                    .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));

            const int yIndent =
                juce::jmin(4, button.proportionOfHeight(0.3f)) - 4;
            const int cornerSize =
                juce::jmin(button.getHeight(), button.getWidth()) / 2;

            const int fontHeight = juce::roundToInt(font.getHeight() * 0.6f);
            const int leftIndent = juce::jmin(
                fontHeight,
                2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
            const int rightIndent = juce::jmin(
                fontHeight,
                2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
            const int textWidth = button.getWidth() - leftIndent - rightIndent;

            if (textWidth > 0)
            {
                g.drawFittedText(button.getButtonText(), leftIndent, -1,
                                 textWidth, button.getHeight() - yIndent * 2,
                                 juce::Justification::centred, 2);
            }
        }

        void positionComboBoxText(juce::ComboBox &box,
                                  juce::Label &label) override
        {
            label.setBounds(1, 2, box.getWidth() - 30, box.getHeight());

            label.setFont(mainFont.withHeight(17));
        }

        void drawComboBox(juce::Graphics &g, const int width, const int height,
                          bool, int, int, int, int,
                          juce::ComboBox &box) override
        {
            constexpr auto cornerSize = 2.f;
            const juce::Rectangle boxBounds(0, 0, width, height);

            g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
            g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

            g.setColour(textColour);
            g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f),
                                   cornerSize, 1.0f);

            const juce::Rectangle arrowZone(static_cast<float>(width) - 17.f,
                                            static_cast<float>(height) * 0.35f,
                                            13.f,
                                            static_cast<float>(height) * 0.3f);

            juce::Path path;

            path.startNewSubPath(arrowZone.getCentreX(), arrowZone.getBottom());
            path.lineTo(arrowZone.getRight(), arrowZone.getY());
            path.lineTo(arrowZone.getX(), arrowZone.getY());
            path.closeSubPath();

            g.setColour(textColour.withAlpha(box.isEnabled() ? 0.9f : 0.2f));
            g.fillPath(path);
        }

        void drawLevelMeter(juce::Graphics &g, const int width,
                            const int height, const float level) override
        {
            constexpr auto outerCornerSize = 3.0f;
            constexpr auto outerBorderWidth = 2.0f;
            constexpr auto totalBlocks = 7;
            constexpr auto spacingFraction = 0.03f;

            g.setColour(backgroundColour);
            g.fillRoundedRectangle(0.0f, 0.0f, static_cast<float>(width),
                                   static_cast<float>(height), outerCornerSize);

            constexpr auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;
            const auto numBlocks =
                juce::roundToInt(static_cast<float>(totalBlocks) * level);

            const auto blockWidth =
                (static_cast<float>(width) - doubleOuterBorderWidth) /
                static_cast<float>(totalBlocks);
            const auto blockHeight =
                static_cast<float>(height) - doubleOuterBorderWidth;

            const auto blockRectWidth =
                (1.0f - 2.0f * spacingFraction) * blockWidth;
            const auto blockRectSpacing = spacingFraction * blockWidth;

            const auto blockCornerSize = 0.1f * blockWidth;

            const auto c = juce::Colours::darkorange;

            for (auto i = 0; i < totalBlocks; ++i)
            {
                if (i >= numBlocks)
                {
                    g.setColour(c.withAlpha(0.5f));
                }
                else
                {
                    g.setColour(i < totalBlocks - 1 ? c : juce::Colours::red);
                }

                g.fillRoundedRectangle(outerBorderWidth +
                                           static_cast<float>(i) * blockWidth +
                                           blockRectSpacing,
                                       outerBorderWidth, blockRectWidth,
                                       blockHeight, blockCornerSize);
            }
        }

        void drawLabel (juce::Graphics& g, juce::Label& label) override
        {
            g.fillAll (label.findColour (juce::Label::backgroundColourId));

            if (! label.isBeingEdited())
            {
                auto alpha = label.isEnabled() ? 1.0f : 0.5f;
                const juce::Font font (getLabelFont (label));

                g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
                g.setFont (font);

                auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds()).translated(0, -3);

                g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                                  juce::jmax (1, (int) ((float) textArea.getHeight() / font.getHeight())),
                                  label.getMinimumHorizontalScale());

                g.setColour (label.findColour (juce::Label::outlineColourId).withMultipliedAlpha (alpha));
            }
            else if (label.isEnabled())
            {
                g.setColour (label.findColour (juce::Label::outlineColourId));
            }

            g.drawRect (label.getLocalBounds());
        }

    private:
        const juce::Font &mainFont;
        juce::Colour backgroundColour;
        juce::Colour backgroundColourBright;
        juce::Colour backgroundColourBrightest;
        juce::Colour backgroundColourDark;

        juce::Colour textColour;
    };

} // namespace vmpc_juce::standalone