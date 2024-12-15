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
        }

        void mouseMove(const juce::MouseEvent& e) override
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
                updateLinkColor(currentlyHoveringLinkIndex, juce::Colours::blue);
            }
            else
            {
                updateLinkColor(currentlyHoveringLinkIndex, juce::Colours::blue);
                updateLinkColor(linkIndex, juce::Colours::lightblue);
            }

            currentlyHoveringLinkIndex = linkIndex;
            repaint();
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            const auto linkIndex = getLinkIndexAtPosition(e.getPosition());

            if (linkIndex == -1)
            {
                return;
            }

            updateLinkColor(linkIndex, juce::Colours::blue);

            juce::URL(links[linkIndex].url).launchInDefaultBrowser();
        }

    private:
        struct Link
        {
            juce::Rectangle<float> bounds;
            juce::String url;
        };

        int getLinkIndexAtPosition(juce::Point<int> p)
        {
            for (int i = 0; i < links.size(); ++i)
            {
                auto& link = links[i];

                if (link.bounds.toNearestIntEdges().contains(p)) return i;

                /*
                 auto screenBounds = link.bounds.translated(getScreenX(), getScreenY());

                if (screenBounds.contains(p + getScreenPosition().toFloat()))
                {
                    return i;
                }
                */
            }

            return -1;
        }

        juce::String rawText;
        juce::AttributedString parsedText;
        std::vector<Link> links;
        const std::function<juce::Font&()> &getNimbusSansScaled;
        int currentlyHoveringLinkIndex = -1;

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
            {
                for (const auto& run : line.runs)
                {
                    if (run->colour == juce::Colours::blue)
                    {
                        const auto xRange = run->getRunBoundsX();
                        const auto y = line.getLineBounds().getY();
                        const auto height = line.getLineBounds().getHeight();
                        const auto linkRect = juce::Rectangle<float>(xRange.getStart(), y, xRange.getLength(), height);

                        links[linkIndex].bounds = linkRect;

                        const auto partialLinkText = parsedText.getText().substring(run->stringRange.getStart(), run->stringRange.getEnd() + 1);
                        currentLinkText.append(partialLinkText, partialLinkText.length());

                        if (currentLinkText == links[linkIndex].url)
                        {
                            ++linkIndex;
                            currentLinkText.clear();
                        }
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
};
