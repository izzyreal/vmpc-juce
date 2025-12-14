#pragma once

#include "standalone/Utils.hpp"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace vmpc_juce::standalone
{
    class MidiOutputSelector final
        : public juce::Component,
          juce::ChangeListener
    {
    public:
        explicit MidiOutputSelector(juce::AudioDeviceManager &dm)
            : deviceManager(dm)
        {
            deviceManager.addChangeListener(this);
            selector.onChange = [&]
            {
                const auto selectedId = selector.getSelectedId();
                jassert(selectedId != 0);

                const auto deviceId =
                    selectedId == -1
                        ? juce::String{}
                        : juce::MidiOutput::getAvailableDevices()[selectedId -
                                                                  1]
                              .identifier;
                deviceManager.setDefaultMidiOutputDevice(deviceId);
            };

            updateListOfDevices();
            addAndMakeVisible(selector);
        }

        ~MidiOutputSelector() override
        {
            deviceManager.removeChangeListener(this);
        }

        void resized() override
        {
            selector.setBounds(getLocalBounds());
        }

    private:
        void updateListOfDevices()
        {
            selector.clear();

            const auto midiOutputs = juce::MidiOutput::getAvailableDevices();

            selector.addItem(Utils::getNoDeviceString(), -1);
            selector.setSelectedId(-1, juce::dontSendNotification);
            selector.addSeparator();

            for (auto [id, midiOutput] : enumerate(midiOutputs, 1))
            {
                selector.addItem(midiOutput.name, id);

                if (midiOutput.identifier ==
                    deviceManager.getDefaultMidiOutputIdentifier())
                {
                    selector.setSelectedId(id, juce::dontSendNotification);
                }
            }
        }

        void changeListenerCallback(juce::ChangeBroadcaster *) override
        {
            updateListOfDevices();
        }

        juce::ComboBox selector;
        juce::AudioDeviceManager &deviceManager;
    };
} // namespace vmpc_juce::standalone