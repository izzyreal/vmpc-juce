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

        ChannelSelectorListBox(const AudioDeviceSetupDetails &setupDetails,
                               BoxType boxType,
                               const juce::String &noItemsText);

        void refresh();

        int getNumRows() override;

        void paintListBoxItem(int row, juce::Graphics &g, int width, int height, bool) override;

        void listBoxItemClicked(int row,
                                const juce::MouseEvent &e) override;

        void listBoxItemDoubleClicked(int row,
                                      const juce::MouseEvent &) override;

        void returnKeyPressed(int row) override;

        void paint(juce::Graphics &g) override;

        int getBestHeight(int maxHeight);

    private:
        const AudioDeviceSetupDetails setup;
        const BoxType type;
        const juce::String noItemsMessage;
        juce::StringArray items;

        static juce::String getNameForChannelPair(const juce::String &name1,
                                                  const juce::String &name2);

        void flipEnablement(int row) const;

        static void flipBit(juce::BigInteger &chans, int index, int minNumber,
                            int maxNumber);

        int getTickX() const;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorListBox)
    };
}
