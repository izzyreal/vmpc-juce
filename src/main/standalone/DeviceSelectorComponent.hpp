#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace vmpc_juce::standalone
{
    class MidiInputListBox;
    class MidiOutputSelector;

    class DeviceSelectorComponent final : public juce::Component,
                                                        juce::ChangeListener
    {
    public:
        DeviceSelectorComponent(
            juce::AudioDeviceManager &deviceManager, int minAudioInputChannels,
            int maxAudioInputChannels, int minAudioOutputChannels,
            int maxAudioOutputChannels, bool showMidiInputOptions,
            bool showMidiOutputSelector, bool showChannelsAsStereoPairs,
            bool hideAdvancedOptionsWithButton);

        ~DeviceSelectorComponent() override;

        juce::AudioDeviceManager &deviceManager;

        void setItemHeight(int itemHeight);

        int getItemHeight() const noexcept
        {
            return itemHeight;
        }

        juce::ListBox *getMidiInputSelectorListBox() const noexcept;

        void resized() override;

        void childBoundsChanged(Component *child) override;

    private:
        void handleBluetoothButton() const;
        void updateDeviceType();
        void changeListenerCallback(juce::ChangeBroadcaster *) override;
        void updateAllControls();

        std::unique_ptr<juce::ComboBox> deviceTypeDropDown;
        std::unique_ptr<juce::Label> deviceTypeDropDownLabel;
        std::unique_ptr<Component> audioDeviceSettingsComp;
        juce::String audioDeviceSettingsCompType;
        int itemHeight = 0;
        const int minOutputChannels, maxOutputChannels, minInputChannels,
            maxInputChannels;
        const bool showChannelsAsStereoPairs;
        const bool hideAdvancedOptionsWithButton;

        juce::Array<juce::MidiDeviceInfo> currentMidiOutputs;
        std::unique_ptr<MidiInputListBox> midiInputsList;
        std::unique_ptr<MidiOutputSelector> midiOutputSelector;
        std::unique_ptr<juce::Label> midiInputsLabel, midiOutputLabel;
        std::unique_ptr<juce::TextButton> bluetoothButton;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
            DeviceSelectorComponent)
    };

} // namespace vmpc_juce::standalone
