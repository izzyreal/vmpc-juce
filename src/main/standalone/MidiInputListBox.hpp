#pragma once

#include "standalone/Utils.hpp"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace vmpc_juce::standalone
{
    class AudioDeviceManager;

    class MidiInputListBox final : public juce::ListBox, juce::ListBoxModel
    {
    public:
        MidiInputListBox(AudioDeviceManager &dm, const juce::String &noItems,
                         const juce::Font &mainFontToUse)
            : ListBox({}, nullptr), deviceManager(dm), noItemsMessage(noItems),
              mainFont(mainFontToUse)
        {
            updateDevices();
            setModel(this);
            setOutlineThickness(1);
        }

        void updateDevices()
        {
            items = juce::MidiInput::getAvailableDevices();
        }

        int getNumRows() override
        {
            return items.size();
        }

        void paintListBoxItem(const int row, juce::Graphics &g, const int width,
                              int height, const bool rowIsSelected) override
        {
            if (juce::isPositiveAndBelow(row, items.size()))
            {
                if (rowIsSelected)
                {
                    g.fillAll(findColour(juce::TextEditor::highlightColourId)
                                  .withMultipliedAlpha(0.3f));
                }

                const auto item = items[row];
                const bool enabled =
                    deviceManager.isMidiInputDeviceEnabled(item.identifier);

                const auto x = getTickX();
                const auto tickW = static_cast<float>(height) * 0.75f;

                getLookAndFeel().drawTickBox(
                    g, *this, static_cast<float>(x) - tickW,
                    (static_cast<float>(height) - tickW) * 0.5f, tickW, tickW,
                    enabled, true, true, false);

                Utils::drawTextLayout(g, *this, item.name,
                                      {x + 5, -1, width - x - 5, height},
                                      enabled, mainFont);
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
                g.drawText(noItemsMessage, 0, 0, getWidth(), getHeight() / 2,
                           juce::Justification::centred, true);
            }
        }

    private:
        AudioDeviceManager &deviceManager;
        const juce::String noItemsMessage;
        juce::Array<juce::MidiDeviceInfo> items;
        const juce::Font &mainFont;

        void flipEnablement(const int row) const
        {
            if (juce::isPositiveAndBelow(row, items.size()))
            {
                const auto identifier = items[row].identifier;
                deviceManager.setMidiInputDeviceEnabled(
                    identifier,
                    !deviceManager.isMidiInputDeviceEnabled(identifier));
            }
        }

        int getTickX() const
        {
            return getRowHeight();
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiInputListBox)
    };
} // namespace vmpc_juce::standalone