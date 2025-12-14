#pragma once

#include "standalone/ChannelSelectorListBox.hpp"
#include "standalone/AudioDeviceSetupDetails.hpp"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace vmpc_juce::standalone
{
    class ChannelSelectorListBox;
    class DeviceSelectorComponent;

    class AudioDeviceSettingsPanel final : public juce::Component,
                                           juce::ChangeListener
    {
    public:
        AudioDeviceSettingsPanel(juce::AudioIODeviceType &t,
                                 const AudioDeviceSetupDetails &setupDetails,
                                 DeviceSelectorComponent &p);

        ~AudioDeviceSettingsPanel() override;

        void resized() override;

        void updateConfig(bool updateOutputDevice, bool updateInputDevice,
                          bool updateSampleRate, bool updateBufferSize);

        bool showDeviceControlPanel() const;

        void showDeviceUIPanel() const;

        void playTestSound() const;

        void updateAllControls();

        void changeListenerCallback(juce::ChangeBroadcaster *) override;

        void resetDevice() const;

    private:
        juce::AudioIODeviceType &type;
        const AudioDeviceSetupDetails setup;
        DeviceSelectorComponent &parent;

        std::unique_ptr<juce::ComboBox> outputDeviceDropDown,
            inputDeviceDropDown, sampleRateDropDown, bufferSizeDropDown;
        std::unique_ptr<juce::Label> outputDeviceLabel, inputDeviceLabel,
            sampleRateLabel, bufferSizeLabel, recordInLabel, stereoOutLabel,
            assignableMixOutLabel;

        std::unique_ptr<juce::TextButton> testButton;
        std::unique_ptr<Component> inputLevelMeter;
        std::unique_ptr<juce::TextButton> showUIButton, resetDeviceButton;

        int findSelectedDeviceIndex(bool isInput) const;
        void updateSelectedInput() const;

        void updateSelectedOutput() const;

        void showCorrectDeviceName(juce::ComboBox *box, bool isInput) const;

        void addNamesToDeviceBox(juce::ComboBox &combo, bool isInputs) const;

        int getLowestY() const;

        void updateControlPanelButton();

        void updateResetButton();

        void updateOutputsComboBox() const;

        void updateInputsComboBox() const;

        void updateSampleRateComboBox(juce::AudioIODevice *currentDevice);

        void updateBufferSizeComboBox(juce::AudioIODevice *currentDevice);

        std::unique_ptr<ChannelSelectorListBox> recordInList, stereoOutList,
            assignableMixOutList;
        juce::ScopedMessageBox messageBox;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeviceSettingsPanel)
    };
} // namespace vmpc_juce::standalone
