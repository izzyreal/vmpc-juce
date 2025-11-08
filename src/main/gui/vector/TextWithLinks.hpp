#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "VmpcTextLayout.hpp"

#include "build_info/VmpcJuceBuildInfo.hpp"

namespace vmpc_juce::gui::vector
{

    class TextWithLinks : public juce::Component
    {
    public:
        TextWithLinks(
            const juce::String &rawTextToUse,
            const std::function<juce::Font &()> &getMainFontScaledToUse)
            : rawText(rawTextToUse), getMainFontScaled(getMainFontScaledToUse)
        {
            setOpaque(true);
            parse();
            setInterceptsMouseClicks(false, false);
            setWantsKeyboardFocus(true);
        }

        void updateFont()
        {
            auto font = getMainFontScaled();
            font.setHeight(font.getHeight() * 1.25f);
            parsedText.setFont(font);
        }

        void paint(juce::Graphics &g) override
        {
            g.fillAll(juce::Colours::white);

            if (selectionStart != -1 && selectionEnd != -1)
            {
                auto start =
                    static_cast<size_t>(std::min(selectionStart, selectionEnd));
                auto end =
                    static_cast<size_t>(std::max(selectionStart, selectionEnd));

                for (size_t i = start; i <= end; ++i)
                {
                    if (i < characterBounds.size())
                    {
                        g.setColour(juce::Colours::lightblue.withAlpha(0.5f)),
                            g.fillRect(characterBounds[i]);
                    }
                }
            }

            VmpcTextLayout layout;
            layout.createLayout(parsedText, static_cast<float>(getWidth()));
            layout.draw(g, getLocalBounds().toFloat());
        }

        int getTextLayoutHeight()
        {
            VmpcTextLayout layout;
            layout.createLayout(parsedText, static_cast<float>(getWidth()));
            return (int)std::ceil(layout.getHeight());
        }

        void resized() override
        {
            VmpcTextLayout layout;
            layout.createLayout(parsedText, static_cast<float>(getWidth()));
            updateLinkBounds(layout);
            updateCharacterBounds(layout);
        }

        void mouseMove(const juce::MouseEvent &e) override
        {
            const auto linkIndex = getLinkIndexAtPosition(e.getPosition());

            if (linkIndex == currentlyHoveringLinkIndex)
            {
                return;
            }

            if (linkIndex >= 0 && currentlyHoveringLinkIndex == -1)
            {
                updateLinkColor(linkIndex, juce::Colours::lightblue);
            }
            else if (linkIndex == -1 && currentlyHoveringLinkIndex >= 0)
            {
                updateLinkColor(currentlyHoveringLinkIndex,
                                juce::Colours::blue);
            }
            else
            {
                updateLinkColor(currentlyHoveringLinkIndex,
                                juce::Colours::blue),
                    updateLinkColor(linkIndex, juce::Colours::lightblue);
            }

            currentlyHoveringLinkIndex = linkIndex;
            repaint();
        }

        void mouseDown(const juce::MouseEvent &e) override
        {
            const auto linkIndex = getLinkIndexAtPosition(e.getPosition());

            if (linkIndex != -1)
            {
                updateLinkColor(linkIndex, juce::Colours::blue),
                    juce::URL(links[static_cast<size_t>(linkIndex)].url)
                        .launchInDefaultBrowser();
            }

            if (e.mods.isShiftDown() && selectionStart != -1)
            {
                selectionEnd = getCharacterIndexAtPosition(e.getPosition());
            }
            else
            {
                selectionStart = getCharacterIndexAtPosition(e.getPosition());
                selectionEnd = -1;
            }

            repaint();
        }

        void updateSelectionEnd(const juce::Point<int> &p)
        {
            selectionEnd = getCharacterIndexAtPosition(p);
            repaint();
        }

        void mouseDrag(const juce::MouseEvent &e) override
        {
            if (e.getPosition().getY() < 0 || selectionStart == -1)
            {
                return;
            }

            updateSelectionEnd(e.getPosition());
        }

        void mouseDoubleClick(const juce::MouseEvent &e) override
        {
            const bool selectLineEnabled = e.getNumberOfClicks() == 3;
            const auto charIndex = getCharacterIndexAtPosition(e.getPosition());
            const auto &text = parsedText.getText();

            if (!std::isalpha(text[charIndex]))
            {
                return;
            }

            const auto checkChar = [&](int i) -> bool
            {
                const bool isAlpha = std::isalpha(text[i]);
                if (!selectLineEnabled && !isAlpha)
                {
                    return false;
                }

                const bool isNewline = text[i] == '\n';
                if (selectLineEnabled && isNewline)
                {
                    return false;
                }

                return true;
            };

            int startIndex = charIndex, endIndex = charIndex;

            for (int i = charIndex; i >= 0; i--)
            {
                if (!checkChar(i))
                {
                    break;
                }
                startIndex = i;
            }

            for (int i = charIndex; i < text.length(); i++)
            {
                if (!checkChar(i))
                {
                    break;
                }
                endIndex = i;
            }

            selectionStart = startIndex;
            selectionEnd = endIndex;
            repaint();
        }

        void mouseUp(const juce::MouseEvent &) override {}

        bool keyPressed(const juce::KeyPress &key) override
        {
            if ((selectionEnd - selectionStart > 0 &&
                 key == juce::KeyPress('c', juce::ModifierKeys::ctrlModifier,
                                       0)) ||
                key ==
                    juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0))
            {
                const auto start = selectionStart > selectionEnd
                                       ? selectionEnd
                                       : selectionStart;
                const auto end = selectionEnd > selectionStart ? selectionEnd
                                                               : selectionStart;

                juce::SystemClipboard::copyTextToClipboard(
                    parsedText.getText().substring(start, end + 1).trimEnd());
                return true;
            }
            return juce::Component::keyPressed(key);
        }

    private:
        struct Link
        {
            std::vector<juce::Rectangle<float>> bounds;
            juce::String url;
            juce::String displayText;
        };

        juce::String rawText;
        juce::AttributedString parsedText;
        std::vector<Link> links;
        std::vector<juce::Rectangle<float>> characterBounds;
        std::vector<juce::Rectangle<float>> lineBounds;

        const std::function<juce::Font &()> &getMainFontScaled;

        int currentlyHoveringLinkIndex = -1;
        int selectionStart = -1;
        int selectionEnd = -1;

        int getLinkIndexAtPosition(juce::Point<int> p)
        {
            for (size_t i = 0; i < links.size(); ++i)
            {
                for (auto &b : links[i].bounds)
                {
                    if (b.toNearestIntEdges().contains(p))
                    {
                        return static_cast<int>(i);
                    }
                }
            }

            return -1;
        }

        int getNthNewlineCharacterIndex(const int n)
        {
            int newlineCount = 0;

            for (int i = 0; i < parsedText.getNumAttributes(); ++i)
            {
                auto &attribute = parsedText.getAttribute(i);
                int start = attribute.range.getStart();
                int end = attribute.range.getEnd();

                for (int j = start; j < end; ++j)
                {
                    if (parsedText.getText()[j] == '\n')
                    {
                        if (newlineCount == n)
                        {
                            return j;
                        }

                        newlineCount++;
                    }
                }
            }

            return parsedText.getText().length() - 1;
        }

        int getCharacterIndexAtPosition(juce::Point<int> p)
        {
            if (p.getY() < 0)
            {
                return 0;
            }
            else if (p.getY() > getHeight())
            {
                return parsedText.getText().length() - 1;
            }

            for (size_t i = 0; i < characterBounds.size(); ++i)
            {
                if (characterBounds[i].contains(p.toFloat()))
                {
                    return static_cast<int>(i);
                }
            }

            const auto lineIndex = getLineIndexAtPosition(p);

            const auto thisLineBounds =
                lineBounds[static_cast<size_t>(lineIndex)];

            const auto lineIndexToUse =
                static_cast<float>(p.getX()) < thisLineBounds.getX()
                    ? lineIndex - 1
                    : lineIndex;

            if (lineIndexToUse < 0)
            {
                return 0;
            }

            return getNthNewlineCharacterIndex(lineIndexToUse);
        }

        void parse()
        {
            links.clear();
            parsedText.clear();

            juce::String remainingText = rawText;

            remainingText = remainingText.replace("<build_version>", vmpc_juce::build_info::getVersionString());
            remainingText =
                remainingText.replace("<build_timestamp>", vmpc_juce::build_info::getTimeStampString());

            while (!remainingText.isEmpty())
            {
                const int start = remainingText.indexOf("<link");
                const int end = remainingText.indexOf("</link>");
                const auto urlStart = remainingText.indexOf(">") + 1;
                const auto urlEnd = end;
                int displayTextEnd = end;

                if (start < 0 || end < 0 || end <= start)
                {
                    parsedText.append(remainingText, juce::Colours::black);
                    break;
                }

                if (start > 0)
                {
                    parsedText.append(remainingText.substring(0, start),
                                      juce::Colours::black);
                }

                int displayTextStart = remainingText.indexOf("displayText=\"");

                if (displayTextStart < 0 || displayTextStart > end)
                {
                    displayTextStart = remainingText.indexOf("<link>") + 6;
                }
                else
                {
                    displayTextStart += juce::String("displayText=\"").length();
                    displayTextEnd = remainingText.indexOf("\">");
                }

                const auto displayText =
                    remainingText.substring(displayTextStart, displayTextEnd);
                const auto urlText = remainingText.substring(urlStart, urlEnd);

                parsedText.append(displayText, juce::Colours::blue);
                links.push_back({/* .bounds = */ {},
                                 /* .url = */ urlText.trim(),
                                 /* .displayText = */ displayText.trim()});

                remainingText = remainingText.substring(end + 7);
            }
        }

        void updateLinkBounds(const VmpcTextLayout &layout)
        {
            int linkIndex = 0;
            juce::String currentLinkText;

            for (auto &l : links)
            {
                l.bounds.clear();
            }

            for (const auto &line : layout)
            {
                for (const auto &run : line.runs)
                {
                    if (run->colour != juce::Colours::blue)
                    {
                        continue;
                    }

                    const auto xRange = run->getRunBoundsX();
                    const auto y = line.getLineBounds().getY();
                    const auto height = line.getLineBounds().getHeight();

                    links[static_cast<size_t>(linkIndex)].bounds.emplace_back(
                        juce::Rectangle<float>(xRange.getStart(), y,
                                               xRange.getLength(), height));

                    const auto partialLinkText = parsedText.getText().substring(
                        run->stringRange.getStart(), run->stringRange.getEnd());

                    currentLinkText.append(
                        partialLinkText,
                        static_cast<size_t>(partialLinkText.length()));

                    if (currentLinkText ==
                        links[static_cast<size_t>(linkIndex)].displayText)
                    {
                        ++linkIndex;
                        currentLinkText.clear();
                    }
                }
            }
        }

        void updateCharacterBounds(const VmpcTextLayout &layout)
        {
            characterBounds.clear();
            lineBounds.clear();

            juce::Range<float> lineBoundsY;

            for (const auto &line : layout)
            {
                lineBounds.push_back(line.getLineBounds().withWidth(
                    static_cast<float>(getWidth())));
                lineBoundsY = line.getLineBoundsY();

                for (const auto &run : line.runs)
                {
                    for (const auto &glyph : run->glyphs)
                    {
                        const auto glyphBounds = juce::Rectangle<float>(
                            glyph.anchor.getX(), lineBoundsY.getStart(),
                            glyph.width, std::ceil(lineBoundsY.getLength()));
                        characterBounds.push_back(glyphBounds);
                    }
                }
            }
        }

        void updateLinkColor(int index, juce::Colour newColour)
        {
            int currentLinkIndex = 0;
            for (int i = 0; i < parsedText.getNumAttributes(); i++)
            {
                auto &a = parsedText.getAttribute(i);
                if (a.colour == juce::Colours::blue ||
                    a.colour == juce::Colours::lightblue)
                {
                    if (currentLinkIndex != index)
                    {
                        currentLinkIndex++;
                        continue;
                    }
                    parsedText.setColour(a.range, newColour);
                    break;
                }
            }
        }

        int getLineIndexAtPosition(const juce::Point<int> &p)
        {
            for (size_t i = 0; i < lineBounds.size(); i++)
            {
                if (lineBounds[i].expanded(5000, 1).toNearestInt().contains(p))
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }
    };

} // namespace vmpc_juce::gui::vector
