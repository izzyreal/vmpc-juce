#pragma once

#include <juce_graphics/juce_graphics.h>

#include <algorithm>

namespace vmpc_juce::gui::vector::disclaimer_layout
{
    inline juce::String getText()
    {
        return juce::String::fromUTF8(
            "MPC® and Akai Professional® are registered trademarks of "
            "inMusic Brands. Inc. This emulator is not affiliated with "
            "inMusic and use of the MPC® and Akai Professional® names has "
            "not been authorized, sponsored or otherwise approved by "
            "inMusic.");
    }

    struct Layout
    {
        juce::Rectangle<float> panelBounds;
        juce::Rectangle<float> contentBounds;
        juce::TextLayout text;
        float styleScale = 1.f;
        float fontHeight = 1.f;
    };

    inline juce::TextLayout createTextLayout(const juce::String &text,
                                             const juce::Font &font,
                                             const float maximumWidth)
    {
        juce::AttributedString attributed{text};
        attributed.setColour(juce::Colours::black);
        attributed.setFont(font);
        attributed.setJustification(juce::Justification::centred);
        attributed.setWordWrap(juce::AttributedString::WordWrap::byWord);

        juce::TextLayout result;
        result.createLayoutWithBalancedLineLengths(attributed,
                                                   std::max(1.f, maximumWidth));
        return result;
    }

    inline Layout calculate(const juce::Rectangle<float> componentBounds,
                            const juce::Font &baseFont,
                            const float requestedScaleMultiplier)
    {
        Layout result;
        result.styleScale = std::max(0.01f, requestedScaleMultiplier);

        const auto minimumComponentDimension =
            std::max(1.f, std::min(componentBounds.getWidth(),
                                   componentBounds.getHeight()));
        const auto shadowInset = 13.f * result.styleScale;
        const auto outerInset =
            std::min(shadowInset, minimumComponentDimension * 0.1f);
        const auto safeBounds = componentBounds.reduced(outerInset);
        const auto minimumSafeDimension = std::max(
            1.f, std::min(safeBounds.getWidth(), safeBounds.getHeight()));
        const auto padding =
            std::min(8.f * result.styleScale, minimumSafeDimension * 0.2f);
        const auto maximumTextWidth =
            std::max(1.f, safeBounds.getWidth() - (padding * 2.f));
        const auto maximumTextHeight =
            std::max(1.f, safeBounds.getHeight() - (padding * 2.f));

        const auto preferredFontHeight =
            std::max(1.f, baseFont.getHeight() * 1.5f * result.styleScale);
        const auto makeLayout = [&](const float fontHeight)
        {
            return createTextLayout(getText(), baseFont.withHeight(fontHeight),
                                    maximumTextWidth);
        };
        const auto fits = [&](const juce::TextLayout &layout)
        {
            return layout.getWidth() <= maximumTextWidth &&
                   layout.getHeight() <= maximumTextHeight;
        };

        result.fontHeight = preferredFontHeight;
        result.text = makeLayout(result.fontHeight);

        if (!fits(result.text))
        {
            auto fittingHeight = 1.f;
            auto overflowingHeight = preferredFontHeight;
            for (auto iteration = 0; iteration < 16; ++iteration)
            {
                const auto candidateHeight =
                    (fittingHeight + overflowingHeight) * 0.5f;
                const auto candidateLayout = makeLayout(candidateHeight);
                if (fits(candidateLayout))
                {
                    fittingHeight = candidateHeight;
                }
                else
                {
                    overflowingHeight = candidateHeight;
                }
            }
            result.fontHeight = fittingHeight;
            result.text = makeLayout(result.fontHeight);
        }

        const auto panelHeight = std::min(
            safeBounds.getHeight(), result.text.getHeight() + (padding * 2.f));
        result.panelBounds = safeBounds.withHeight(panelHeight)
                                 .withCentre(safeBounds.getCentre());
        result.contentBounds = result.panelBounds.reduced(padding);
        return result;
    }
} // namespace vmpc_juce::gui::vector::disclaimer_layout
