#include <juce_gui_basics/juce_gui_basics.h>

#include "version.h"

class TextWithLinks : public juce::Component
{
    public:
        TextWithLinks(const juce::String& rawTextToUse, const std::function<juce::Font&()> &getNimbusSansScaledToUse)
            : rawText(rawTextToUse), getNimbusSansScaled(getNimbusSansScaledToUse)
    {
        setOpaque(true);
    }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::white);

            auto font = getNimbusSansScaled();
            font.setHeight(font.getHeight() * 1.4f);

            juce::TextLayout layout;
            layout.createLayout(parsedText, getWidth() * 100);
            layout.draw(g, getLocalBounds().toFloat());
        }

        void resized() override
        {
            parse();
            juce::TextLayout layout;
            layout.createLayout(parsedText, getWidth() * 100);
            setSize(getWidth(), (int)std::ceil(layout.getHeight()));
        }

        void mouseMove(const juce::MouseEvent& e) override
        {
            const auto linkIndex = getLinkIndexAtPosition(e.position);

            for (int i = 0; i < links.size(); i++)
            {
                updateLinkColor(i, juce::Colours::blue);
            }

            if (linkIndex != -1)
            {
                updateLinkColor(linkIndex, juce::Colours::lightblue);
            }

            repaint();
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            const auto linkIndex = getLinkIndexAtPosition(e.position);
            
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

        int getLinkIndexAtPosition(const juce::Point<float> &p)
        {
            for (int i = 0; i < links.size(); ++i)
            {
                auto& link = links[i];

                auto screenBounds = link.bounds.translated(getScreenX(), getScreenY());

                if (screenBounds.contains(p + getScreenPosition().toFloat()))
                {
                    return i;
                }
            }

            return -1;
        }

        juce::String rawText;
        juce::AttributedString parsedText;
        std::vector<Link> links;
        const std::function<juce::Font&()> &getNimbusSansScaled;

        void parse()
        {
            links.clear();
            parsedText.clear();

            auto font = getNimbusSansScaled();
            font.setHeight(font.getHeight() * 1.4f);

            juce::String remainingText = rawText;
            remainingText = remainingText.replace("<version>", version::get());

            while (!remainingText.isEmpty())
            {
                int start = remainingText.indexOf("<link>");
                int end = remainingText.indexOf("</link>");

                if (start < 0 || end < 0 || end <= start)
                {
                    parsedText.append(remainingText, font, juce::Colours::black);
                    break;
                }

                if (start > 0)
                {
                    parsedText.append(remainingText.substring(0, start), font, juce::Colours::black);
                }

                juce::String linkText = remainingText.substring(start + 6, end);
                parsedText.append(linkText, font, juce::Colours::blue);

                links.push_back({ {}, linkText });

                remainingText = remainingText.substring(end + 7);
            }

            juce::TextLayout layout;
            layout.createLayout(parsedText, getWidth() * 100);

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
