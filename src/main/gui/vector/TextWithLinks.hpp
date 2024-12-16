#include <juce_gui_basics/juce_gui_basics.h>
#include "version.h"

class TextWithLinks : public juce::Component
{
    public:
        TextWithLinks(const juce::String& rawTextToUse, const std::function<juce::Font&()> &getNimbusSansScaledToUse)
            : rawText(rawTextToUse), getNimbusSansScaled(getNimbusSansScaledToUse)
        {
            setOpaque(true);
            parse();
            setInterceptsMouseClicks(false, false);
        }

        void updateFont()
        {
            auto font = getNimbusSansScaled();
            font.setHeight(font.getHeight() * 1.4f);
            parsedText.setFont(font);
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::white);

            if (selectionStart != -1 && selectionEnd != -1)
            {
                int start = std::min(selectionStart, selectionEnd);
                int end = std::max(selectionStart, selectionEnd);

                for (int i = start; i <= end; ++i)
                {
                    if (i < characterBounds.size())
                        g.setColour(juce::Colours::lightblue.withAlpha(0.5f)), g.fillRect(characterBounds[i]);
                }
            }

            juce::TextLayout layout;
            layout.createLayout(parsedText, getWidth());
            layout.draw(g, getLocalBounds().toFloat());
        }

        int getTextLayoutHeight()
        {
            juce::TextLayout layout;
            layout.createLayout(parsedText, getWidth());
            return (int)std::ceil(layout.getHeight());
        }

        void resized() override
        {
            juce::TextLayout layout;
            layout.createLayout(parsedText, getWidth());
            updateLinkBounds(layout);
            updateCharacterBounds(layout);
        }

        void mouseMove(const juce::MouseEvent& e) override
        {
            const auto linkIndex = getLinkIndexAtPosition(e.getPosition());
            if (linkIndex == currentlyHoveringLinkIndex) return;
            if (linkIndex >= 0 && currentlyHoveringLinkIndex == -1)
                updateLinkColor(linkIndex, juce::Colours::lightblue);
            else if (linkIndex == -1 && currentlyHoveringLinkIndex >= 0)
                updateLinkColor(currentlyHoveringLinkIndex, juce::Colours::blue);
            else
                updateLinkColor(currentlyHoveringLinkIndex, juce::Colours::blue), updateLinkColor(linkIndex, juce::Colours::lightblue);
            currentlyHoveringLinkIndex = linkIndex;
            repaint();
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            const auto linkIndex = getLinkIndexAtPosition(e.getPosition());
            if (linkIndex != -1) updateLinkColor(linkIndex, juce::Colours::blue), juce::URL(links[linkIndex].url).launchInDefaultBrowser();
            selectionStart = getCharacterIndexAtPosition(e.getPosition());
            selectionEnd = -1;
            repaint();
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (e.getPosition().getY() < 0 || selectionStart == -1)
                return;

            selectionEnd = getCharacterIndexAtPosition(e.getPosition());
            repaint();
        }

        void mouseUp(const juce::MouseEvent& e) override { }

    private:
        struct Link
        {
            juce::Rectangle<float> bounds;
            juce::String url;
        };

        juce::String rawText;
        juce::AttributedString parsedText;
        std::vector<Link> links;
        std::vector<juce::Rectangle<float>> characterBounds;
        std::vector<juce::Rectangle<float>> lineBounds;
        const std::function<juce::Font&()> &getNimbusSansScaled;
        int currentlyHoveringLinkIndex = -1;
        int selectionStart = -1;
        int selectionEnd = -1;

        int getLinkIndexAtPosition(juce::Point<int> p)
        {
            for (int i = 0; i < links.size(); ++i)
            {
                if (links[i].bounds.toNearestIntEdges().contains(p))
                {
                    return i;
                }
            }

            return -1;
        }

        int getNthNewlineCharacterIndex(const int n)
        {
            int newlineCount = 0;

            for (int i = 0; i < parsedText.getNumAttributes(); ++i)
            {
                auto& attribute = parsedText.getAttribute(i);
                int start = attribute.range.getStart();
                int end = attribute.range.getEnd();

                for (int j = start; j < end; ++j)
                {
                    if (parsedText.getText()[j] == '\n')
                    {
                        newlineCount++;

                        if (newlineCount == n)
                        {
                            return j;
                        }
                    }
                }
            }

            return -1;
        }

        int getCharacterIndexAtPosition(juce::Point<int> p)
        {
            for (int i = 0; i < characterBounds.size(); ++i)
            {
                if (characterBounds[i].contains(p.toFloat()))
                {
                    return i;
                }
            }

            const auto lineIndex = getLineIndexAtPosition(p);

            const auto thisLineBounds = lineBounds[lineIndex];

            const auto newLineIndex = p.getX() < thisLineBounds.getX() ? lineIndex - 1 : lineIndex + 1;

            if (newLineIndex < 0)
            {
                return 0;
            }

            return getNthNewlineCharacterIndex(newLineIndex);
        }

        void parse()
        {
            links.clear();
            parsedText.clear();
            juce::String remainingText = rawText;
            remainingText = remainingText.replace("<version>", version::get());
            remainingText = remainingText.replace("<build>", version::getTimeStamp());
            while (!remainingText.isEmpty())
            {
                int start = remainingText.indexOf("<link>");
                int end = remainingText.indexOf("</link>");

                if (start < 0 || end < 0 || end <= start)
                {
                    parsedText.append(remainingText, juce::Colours::black);
                    break;
                }

                if (start > 0)
                {
                    parsedText.append(remainingText.substring(0, start), juce::Colours::black);
                }

                juce::String linkText = remainingText.substring(start + 6, end);
                parsedText.append(linkText, juce::Colours::blue);
                links.push_back({ {}, linkText });
                remainingText = remainingText.substring(end + 7);
            }
        }

        void updateLinkBounds(const juce::TextLayout &layout)
        {
            int linkIndex = 0;
            juce::String currentLinkText;
            for (const auto& line : layout)
                for (const auto& run : line.runs)
                    if (run->colour == juce::Colours::blue)
                    {
                        const auto xRange = run->getRunBoundsX();
                        const auto y = line.getLineBounds().getY();
                        const auto height = line.getLineBounds().getHeight();
                        const auto linkRect = juce::Rectangle<float>(xRange.getStart(), y, xRange.getLength(), height);
                        links[linkIndex].bounds = linkRect;
                        const auto partialLinkText = parsedText.getText().substring(run->stringRange.getStart(), run->stringRange.getEnd() + 1);
                        currentLinkText.append(partialLinkText, partialLinkText.length());
                        if (currentLinkText == links[linkIndex].url) ++linkIndex, currentLinkText.clear();
                    }
        }

        void updateCharacterBounds(const juce::TextLayout &layout)
        {
            characterBounds.clear();
            lineBounds.clear();

            juce::Range<float> lineBoundsY;

            for (const auto& line : layout)
            {
                lineBounds.push_back(line.getLineBounds().withWidth(getWidth()));
                lineBoundsY = line.getLineBoundsY();

                for (const auto& run : line.runs)
                {
                    for (const auto& glyph : run->glyphs)
                    {
                        const auto glyphBounds = juce::Rectangle<float>(
                                glyph.anchor.getX(),
                                lineBoundsY.getStart(),
                                glyph.width,
                                std::ceil(lineBoundsY.getLength()));
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
                if (a.colour == juce::Colours::blue || a.colour == juce::Colours::lightblue)
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
            for (int i = 0; i < lineBounds.size(); i++)
            {
                if (lineBounds[i].expanded(1).toNearestInt().contains(p))
                {
                    return i;
                }
            }
            return -1;
        }

        std::pair<int, int> getCharacterRangeOfLineIndex(const int lineIndex)
        {
            if (lineIndex < 0 || lineIndex >= lineBounds.size())
                return { -1, -1 };

            int start = -1;
            int end = -1;

            for (int i = 0; i < characterBounds.size(); ++i)
            {
                if (characterBounds[i].getY() == lineBounds[lineIndex].getY())
                {
                    if (start == -1)
                        start = i;
                    end = i;
                }
            }

            return { start, end };
        }
};
