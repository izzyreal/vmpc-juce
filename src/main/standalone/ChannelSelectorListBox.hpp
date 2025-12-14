#pragma once

#include "standalone/AudioDeviceSetupDetails.hpp"
#include "standalone/Utils.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::standalone
{
    class ChannelSelectorListBox final : public juce::ListBox,
                                         juce::ListBoxModel
    {
    public:
        enum BoxType
        {
            audioInputType,
            audioOutputType
        };

        ChannelSelectorListBox(const AudioDeviceSetupDetails &setupDetails,
                               const BoxType boxType,
                               const juce::String &noItemsText)
            : ListBox({}, nullptr), setup(setupDetails), type(boxType),
              noItemsMessage(noItemsText)
        {
            refresh();
            setModel(this);
            setOutlineThickness(1);
        }

        void refresh()
        {
            items.clear();

            if (auto *currentDevice =
                    setup.manager->getCurrentAudioDevice())
            {
                if (type == audioInputType)
                {
                    items = currentDevice->getInputChannelNames();
                }
                else if (type == audioOutputType)
                {
                    items = currentDevice->getOutputChannelNames();
                }

                if (setup.useStereoPairs)
                {
                    juce::StringArray pairs;

                    for (int i = 0; i < items.size(); i += 2)
                    {
                        auto &name = items[i];

                        if (i + 1 >= items.size())
                        {
                            pairs.add(name.trim());
                        }
                        else
                        {
                            pairs.add(
                                getNameForChannelPair(name, items[i + 1]));
                        }
                    }

                    items = pairs;
                }
            }

            updateContent();
            repaint();
        }

        int getNumRows() override
        {
            return items.size();
        }

        void paintListBoxItem(const int row, juce::Graphics &g,
                              const int width, int height, bool) override
        {
            if (juce::isPositiveAndBelow(row, items.size()))
            {
                g.fillAll(findColour(backgroundColourId));

                const auto item = items[row];
                bool enabled = false;
                const auto config = setup.manager->getAudioDeviceSetup();

                if (setup.useStereoPairs)
                {
                    if (type == audioInputType)
                    {
                        enabled = config.inputChannels[row * 2] ||
                                  config.inputChannels[row * 2 + 1];
                    }
                    else if (type == audioOutputType)
                    {
                        enabled = config.outputChannels[row * 2] ||
                                  config.outputChannels[row * 2 + 1];
                    }
                }
                else
                {
                    if (type == audioInputType)
                    {
                        enabled = config.inputChannels[row];
                    }
                    else if (type == audioOutputType)
                    {
                        enabled = config.outputChannels[row];
                    }
                }

                const auto x = getTickX();
                const auto tickW = static_cast<float>(height) * 0.75f;

                getLookAndFeel().drawTickBox(
                    g, *this, static_cast<float>(x) - tickW,
                    (static_cast<float>(height) - tickW) * 0.5f, tickW,
                    tickW, enabled, true, true, false);

                drawTextLayout(g, *this, item,
                               {x + 5, 0, width - x - 5, height}, enabled);
            }
        }

        void listBoxItemClicked(const int row,
                                const juce::MouseEvent &e) override
        {
            selectRow(row);

            if (e.x < getTickX())
            {
                flipEnablement(row);
            }
        }

        void listBoxItemDoubleClicked(const int row,
                                      const juce::MouseEvent &) override
        {
            flipEnablement(row);
        }

        void returnKeyPressed(const int row) override
        {
            flipEnablement(row);
        }

        void paint(juce::Graphics &g) override
        {
            ListBox::paint(g);

            if (items.isEmpty())
            {
                g.setColour(juce::Colours::grey);
                g.setFont(0.5f * static_cast<float>(getRowHeight()));
                g.drawText(noItemsMessage, 0, 0, getWidth(),
                           getHeight() / 2, juce::Justification::centred,
                           true);
            }
        }

        int getBestHeight(const int maxHeight)
        {
            return getRowHeight() *
                       juce::jlimit(
                           2, juce::jmax(2, maxHeight / getRowHeight()),
                           getNumRows()) +
                   getOutlineThickness() * 2;
        }

    private:
        const AudioDeviceSetupDetails setup;
        const BoxType type;
        const juce::String noItemsMessage;
        juce::StringArray items;

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

        void flipEnablement(const int row) const
        {
            jassert(type == audioInputType || type == audioOutputType);

            if (juce::isPositiveAndBelow(row, items.size()))
            {
                auto config = setup.manager->getAudioDeviceSetup();

                if (setup.useStereoPairs)
                {
                    juce::BigInteger bits;
                    auto &original = type == audioInputType
                                         ? config.inputChannels
                                         : config.outputChannels;

                    for (int i = 0; i < 256; i += 2)
                    {
                        bits.setBit(i / 2, original[i] || original[i + 1]);
                    }

                    if (type == audioInputType)
                    {
                        config.useDefaultInputChannels = false;
                        flipBit(bits, row, setup.minNumInputChannels / 2,
                                setup.maxNumInputChannels / 2);
                    }
                    else
                    {
                        config.useDefaultOutputChannels = false;
                        flipBit(bits, row, setup.minNumOutputChannels / 2,
                                setup.maxNumOutputChannels / 2);
                    }

                    for (int i = 0; i < 256; ++i)
                    {
                        original.setBit(i, bits[i / 2]);
                    }
                }
                else
                {
                    if (type == audioInputType)
                    {
                        config.useDefaultInputChannels = false;
                        flipBit(config.inputChannels, row,
                                setup.minNumInputChannels,
                                setup.maxNumInputChannels);
                    }
                    else
                    {
                        config.useDefaultOutputChannels = false;
                        flipBit(config.outputChannels, row,
                                setup.minNumOutputChannels,
                                setup.maxNumOutputChannels);
                    }
                }

                setup.manager->setAudioDeviceSetup(config, true);
            }
        }

        static void flipBit(juce::BigInteger &chans, const int index,
                            const int minNumber, const int maxNumber)
        {
            const auto numActive = chans.countNumberOfSetBits();

            if (chans[index])
            {
                if (numActive > minNumber)
                {
                    chans.setBit(index, false);
                }
            }
            else
            {
                if (numActive >= maxNumber)
                {
                    const auto firstActiveChan = chans.findNextSetBit(0);
                    chans.clearBit(index > firstActiveChan
                                       ? firstActiveChan
                                       : chans.getHighestBit());
                }

                chans.setBit(index, true);
            }
        }

        int getTickX() const
        {
            return getRowHeight();
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorListBox)
    };
}
