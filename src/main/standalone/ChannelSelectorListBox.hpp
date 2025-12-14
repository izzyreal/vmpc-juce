#pragma once

#include "standalone/AudioDeviceSetupDetails.hpp"

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

        ChannelSelectorListBox(bool useStereoPairs,
                               const AudioDeviceSetupDetails &setupDetails,
                               BoxType boxType,
                               int channelOffset,
                               int maxChannelCount,
                               const std::vector<std::string> &&nameSuffixes = {});

        void refresh();

        int getNumRows() override;

        void paintListBoxItem(int row, juce::Graphics &g, int width, int height,
                              bool) override;

        void listBoxItemClicked(int row, const juce::MouseEvent &e) override;

        void listBoxItemDoubleClicked(int row,
                                      const juce::MouseEvent &) override;

        void returnKeyPressed(int row) override;

        int getBestHeight(int maxHeight);

    private:
        const AudioDeviceSetupDetails setup;
        const BoxType type;
        juce::StringArray items;
        const bool useStereoPairs;
        int channelOffset;
        int maxChannelCount;
        const std::vector<std::string> nameSuffixes;

        void flipEnablement(int row) const;

        static void flipBit(juce::BigInteger &chans, int index, int minNumber,
                            int maxNumber);

        int getTickX() const;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorListBox)
    };
} // namespace vmpc_juce::standalone
