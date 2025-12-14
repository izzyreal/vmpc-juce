#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::standalone
{
    class Utils
    {
    public:
        static juce::String getNoDeviceString()
        {
            return "<< none >>";
        }

        static void drawTextLayout(juce::Graphics &g,
                                   const juce::Component &owner,
                                   const juce::StringRef &text,
                                   const juce::Rectangle<int> &textBounds,
                                   const bool enabled)
        {
            const auto textColour =
                owner.findColour(juce::ListBox::textColourId, true)
                    .withMultipliedAlpha(enabled ? 1.0f : 0.6f);

            juce::AttributedString attributedString{text};
            attributedString.setColour(textColour);
            attributedString.setFont(
                static_cast<float>(textBounds.getHeight()) * 0.6f);
            attributedString.setJustification(juce::Justification::centredLeft);
            attributedString.setWordWrap(
                juce::AttributedString::WordWrap::none);

            juce::TextLayout textLayout;
            textLayout.createLayout(attributedString,
                                    static_cast<float>(textBounds.getWidth()),
                                    static_cast<float>(textBounds.getHeight()));
            textLayout.draw(g, textBounds.toFloat());
        }

        static juce::String getNameForChannelPair(const juce::String &name1,
                                           const juce::String &name2)
        {
            juce::String commonBit;

            for (int j = 0; j < name1.length(); ++j)
            {
                if (name1.substring(0, j).equalsIgnoreCase(
                        name2.substring(0, j)))
                {
                    commonBit = name1.substring(0, j);
                }
            }

            // Make sure we only split the name at a space, because
            // otherwise, things like "input 11" + "input 12" would become
            // "input 11 + 2"
            while (commonBit.isNotEmpty() &&
                   !juce::CharacterFunctions::isWhitespace(
                       commonBit.getLastCharacter()))
            {
                commonBit = commonBit.dropLastCharacters(1);
            }

            return name1.trim() + " + " +
                   name2.substring(commonBit.length()).trim();
        }
    };
} // namespace vmpc_juce::standalone
