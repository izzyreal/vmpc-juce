#include "standalone/ChannelSelectorListBox.hpp"

#include "standalone/AudioDeviceManager.hpp"
#include "standalone/Utils.hpp"

using namespace vmpc_juce::standalone;

ChannelSelectorListBox::ChannelSelectorListBox(
    const bool useStereoPairsToUse, const AudioDeviceSetupDetails &setupDetails,
    const BoxType boxType, const int channelOffsetToUse,
    const int maxChannelCountToUse, const juce::Font &mainFontToUse,
    const std::vector<std::string> &&nameSuffixesToUse)
    : ListBox({}, nullptr), setup(setupDetails), type(boxType),
      useStereoPairs(useStereoPairsToUse), channelOffset(channelOffsetToUse),
      maxChannelCount(maxChannelCountToUse), nameSuffixes(nameSuffixesToUse),
      mainFont(mainFontToUse)
{
    refresh();
    setModel(this);
    setOutlineThickness(1);
}

void ChannelSelectorListBox::refresh()
{
    items.clear();

    if (auto *currentDevice = setup.manager->getCurrentAudioDevice())
    {
        if (type == audioInputType)
        {
            items = currentDevice->getInputChannelNames();
        }
        else if (type == audioOutputType)
        {
            items = currentDevice->getOutputChannelNames();
        }

        items.removeRange(channelOffset + maxChannelCount, items.size());
        items.removeRange(0, channelOffset);

        if (useStereoPairs)
        {
            juce::StringArray pairs;

            for (int i = channelOffset;
                 i < std::min(items.size(), channelOffset + maxChannelCount);
                 i += 2)
            {
                auto &name = items[i];

                if (i + 1 >= items.size())
                {
                    pairs.add(name.trim());
                }
                else
                {
                    pairs.add(Utils::getNameForChannelPair(name, items[i + 1]));
                }
            }

            items = pairs;
        }

        for (int i = 0;
             i < std::min(static_cast<int>(nameSuffixes.size()), items.size());
             ++i)
        {
            items.getReference(i) =
                items[i] + " <- " + nameSuffixes[static_cast<size_t>(i)];
        }
    }

    updateContent();
    repaint();
}

int ChannelSelectorListBox::getNumRows()
{
    return items.size();
}

void ChannelSelectorListBox::paintListBoxItem(const int row, juce::Graphics &g,
                                              const int width, int height, bool)
{
    if (juce::isPositiveAndBelow(row, items.size()))
    {
        g.fillAll(findColour(backgroundColourId));

        const auto item = items[row];
        bool enabled = false;
        const auto config = setup.manager->getAudioDeviceSetup();

        if (useStereoPairs)
        {
            if (type == audioInputType)
            {
                enabled = config.inputChannels[row * 2 + channelOffset] ||
                          config.inputChannels[row * 2 + 1 + channelOffset];
            }
            else if (type == audioOutputType)
            {
                enabled = config.outputChannels[row * 2 + channelOffset] ||
                          config.outputChannels[row * 2 + 1 + channelOffset];
            }
        }
        else
        {
            if (type == audioInputType)
            {
                enabled = config.inputChannels[row + channelOffset];
            }
            else if (type == audioOutputType)
            {
                enabled = config.outputChannels[row + channelOffset];
            }
        }

        const auto x = getTickX();
        const auto tickW = static_cast<float>(height) * 0.75f;

        getLookAndFeel().drawTickBox(g, *this, static_cast<float>(x) - tickW,
                                     (static_cast<float>(height) - tickW) *
                                         0.5f,
                                     tickW, tickW, enabled, true, true, false);

        Utils::drawTextLayout(g, *this, item,
                              {x + 5, -1, width - x - 5, height}, enabled,
                              mainFont);
    }
}

void ChannelSelectorListBox::listBoxItemClicked(const int row,
                                                const juce::MouseEvent &e)
{
    selectRow(row);

    if (e.x < getTickX())
    {
        flipEnablement(row);
    }
}

void ChannelSelectorListBox::listBoxItemDoubleClicked(const int row,
                                                      const juce::MouseEvent &)
{
    flipEnablement(row);
}

void ChannelSelectorListBox::returnKeyPressed(const int row)
{
    flipEnablement(row);
}

int ChannelSelectorListBox::getBestHeight(const int maxHeight)
{
    return getRowHeight() *
               juce::jlimit(1, juce::jmax(1, maxHeight / getRowHeight()),
                            getNumRows()) +
           getOutlineThickness() * 2;
}

void ChannelSelectorListBox::flipEnablement(const int row) const
{
    jassert(type == audioInputType || type == audioOutputType);

    if (juce::isPositiveAndBelow(row, items.size()))
    {
        auto config = setup.manager->getAudioDeviceSetup();

        if (useStereoPairs)
        {
            juce::BigInteger bits;
            auto &original = type == audioInputType ? config.inputChannels
                                                    : config.outputChannels;

            for (int i = 0; i < 256; i += 2)
            {
                bits.setBit(i / 2, original[i] || original[i + 1]);
            }

            if (type == audioInputType)
            {
                config.useDefaultInputChannels = false;
                flipBit(bits, row + channelOffset,
                        setup.minNumInputChannels / 2,
                        setup.maxNumInputChannels / 2);
            }
            else
            {
                config.useDefaultOutputChannels = false;
                flipBit(bits, row + channelOffset,
                        setup.minNumOutputChannels / 2,
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
                flipBit(config.inputChannels, row + channelOffset,
                        setup.minNumInputChannels, setup.maxNumInputChannels);
            }
            else
            {
                config.useDefaultOutputChannels = false;
                flipBit(config.outputChannels, row + channelOffset,
                        setup.minNumOutputChannels, setup.maxNumOutputChannels);
            }
        }

        setup.manager->setAudioDeviceSetup(config, true);
    }
}

void ChannelSelectorListBox::flipBit(juce::BigInteger &chans, const int index,
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
            chans.clearBit(index > firstActiveChan ? firstActiveChan
                                                   : chans.getHighestBit());
        }

        chans.setBit(index, true);
    }
}

int ChannelSelectorListBox::getTickX() const
{
    return getRowHeight();
}
