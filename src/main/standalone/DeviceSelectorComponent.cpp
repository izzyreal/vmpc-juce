#include "standalone/DeviceSelectorComponent.hpp"

#include "standalone/AudioDeviceManager.hpp"
#include "standalone/MidiInputListBox.hpp"
#include "standalone/MidiOutputSelector.hpp"
#include "standalone/AudioDeviceSetupDetails.hpp"
#include "standalone/AudioDeviceSettingsPanel.hpp"

#include <juce_audio_utils/juce_audio_utils.h>

namespace vmpc_juce::standalone
{
    DeviceSelectorComponent::DeviceSelectorComponent(
        AudioDeviceManager &deviceManagerToUse,
        const int minInputChannelsToUse, const int maxInputChannelsToUse,
        const int minOutputChannelsToUse, const int maxOutputChannelsToUse)
        : deviceManager(deviceManagerToUse), itemHeight(24),
          minOutputChannels(minOutputChannelsToUse),
          maxOutputChannels(maxOutputChannelsToUse),
          minInputChannels(minInputChannelsToUse),
          maxInputChannels(maxInputChannelsToUse)
    {
        jassert(minOutputChannels >= 0 &&
                minOutputChannels <= maxOutputChannels);
        jassert(minInputChannels >= 0 && minInputChannels <= maxInputChannels);

        const juce::OwnedArray<juce::AudioIODeviceType> &types =
            deviceManager.getAvailableDeviceTypes();

        if (types.size() > 1)
        {
            deviceTypeDropDown = std::make_unique<juce::ComboBox>();

            for (int i = 0; i < types.size(); ++i)
            {
                deviceTypeDropDown->addItem(
                    types.getUnchecked(i)->getTypeName(), i + 1);
            }

            addAndMakeVisible(deviceTypeDropDown.get());
            deviceTypeDropDown->onChange = [this]
            {
                updateDeviceType();
            };

            deviceTypeDropDownLabel = std::make_unique<juce::Label>(
                juce::String{}, "Audio device type:");
            deviceTypeDropDownLabel->setJustificationType(
                juce::Justification::centredRight);
            deviceTypeDropDownLabel->attachToComponent(deviceTypeDropDown.get(),
                                                       true);
        }

        midiInputsList = std::make_unique<MidiInputListBox>(
            deviceManager, "(No MIDI inputs available)");
        addAndMakeVisible(midiInputsList.get());

        midiInputsLabel = std::make_unique<juce::Label>(juce::String{},
                                                        "Active MIDI inputs:");
        midiInputsLabel->setJustificationType(juce::Justification::topRight);
        midiInputsLabel->attachToComponent(midiInputsList.get(), true);

        if (juce::BluetoothMidiDevicePairingDialogue::isAvailable())
        {
            bluetoothButton = std::make_unique<juce::TextButton>(
                "Bluetooth MIDI", "Scan for bluetooth MIDI devices");
            addAndMakeVisible(bluetoothButton.get());
            bluetoothButton->onClick = [this]
            {
                handleBluetoothButton();
            };
        }

        midiOutputSelector =
            std::make_unique<MidiOutputSelector>(deviceManager);
        addAndMakeVisible(midiOutputSelector.get());

        midiOutputLabel = std::make_unique<juce::Label>("lm", "MIDI Output:");
        midiOutputLabel->attachToComponent(midiOutputSelector.get(), true);

        deviceManager.addChangeListener(this);
        updateAllControls();
    }

    DeviceSelectorComponent::~DeviceSelectorComponent()
    {
        deviceManager.removeChangeListener(this);
    }

    void DeviceSelectorComponent::setItemHeight(const int newItemHeight)
    {
        itemHeight = newItemHeight;
        resized();
    }

    void DeviceSelectorComponent::resized()
    {
        juce::Rectangle r(proportionOfWidth(0.35f), 15, proportionOfWidth(0.6f),
                          3000);
        const auto space = itemHeight / 4;

        if (deviceTypeDropDown != nullptr)
        {
            deviceTypeDropDown->setBounds(r.removeFromTop(itemHeight));
            r.removeFromTop(space * 3);
        }

        if (audioDeviceSettingsComp != nullptr)
        {
            audioDeviceSettingsComp->resized();
            audioDeviceSettingsComp->setBounds(
                r.removeFromTop(audioDeviceSettingsComp->getHeight())
                    .withX(0)
                    .withWidth(getWidth()));
            r.removeFromTop(space);
        }

        if (midiInputsList != nullptr)
        {
            midiInputsList->setRowHeight(juce::jmin(22, itemHeight));
            midiInputsList->setBounds(
                r.removeFromTop(midiInputsList->getBestHeight(
                    juce::jmin(itemHeight * 8,
                               getHeight() - r.getY() - space - itemHeight))));
            r.removeFromTop(space);
        }

        if (bluetoothButton != nullptr)
        {
            bluetoothButton->setBounds(r.removeFromTop(24));
            r.removeFromTop(space);
        }

        if (midiOutputSelector != nullptr)
        {
            midiOutputSelector->setBounds(r.removeFromTop(itemHeight));
        }

        r.removeFromTop(itemHeight);
        setSize(getWidth(), r.getY());
    }

    void DeviceSelectorComponent::childBoundsChanged(Component *child)
    {
        if (child == audioDeviceSettingsComp.get())
        {
            resized();
        }
    }

    void DeviceSelectorComponent::updateDeviceType()
    {
        if (const auto *type = deviceManager.getAvailableDeviceTypes()
                                   [deviceTypeDropDown->getSelectedId() - 1])
        {
            audioDeviceSettingsComp.reset();
            deviceManager.setCurrentAudioDeviceType(type->getTypeName(), true);
            updateAllControls(); // needed in case the type hasn't actually
                                 // changed
        }
    }

    void
    DeviceSelectorComponent::changeListenerCallback(juce::ChangeBroadcaster *)
    {
        updateAllControls();
    }

    void DeviceSelectorComponent::updateAllControls()
    {
        if (deviceTypeDropDown != nullptr)
        {
            deviceTypeDropDown->setText(
                deviceManager.getCurrentAudioDeviceType(),
                juce::dontSendNotification);
        }

        if (audioDeviceSettingsComp == nullptr ||
            audioDeviceSettingsCompType !=
                deviceManager.getCurrentAudioDeviceType())
        {
            audioDeviceSettingsCompType =
                deviceManager.getCurrentAudioDeviceType();
            audioDeviceSettingsComp.reset();

            if (auto *type =
                    deviceManager.getAvailableDeviceTypes()
                        [deviceTypeDropDown == nullptr
                             ? 0
                             : deviceTypeDropDown->getSelectedId() - 1])
            {
                AudioDeviceSetupDetails details;
                details.manager = &deviceManager;
                details.minNumInputChannels = minInputChannels;
                details.maxNumInputChannels = maxInputChannels;
                details.minNumOutputChannels = minOutputChannels;
                details.maxNumOutputChannels = maxOutputChannels;

                audioDeviceSettingsComp =
                    std::make_unique<AudioDeviceSettingsPanel>(*type, details,
                                                               *this);
                addAndMakeVisible(audioDeviceSettingsComp.get());
            }
        }

        if (midiInputsList != nullptr)
        {
            midiInputsList->updateDevices();
            midiInputsList->updateContent();
            midiInputsList->repaint();
        }

        resized();
    }

    void DeviceSelectorComponent::handleBluetoothButton() const
    {
        if (juce::RuntimePermissions::isGranted(
                juce::RuntimePermissions::bluetoothMidi))
        {
            juce::BluetoothMidiDevicePairingDialogue::open();
        }
        else
        {
            juce::RuntimePermissions::request(
                juce::RuntimePermissions::bluetoothMidi,
                [](auto)
                {
                    if (juce::RuntimePermissions::isGranted(
                            juce::RuntimePermissions::bluetoothMidi))
                    {
                        juce::BluetoothMidiDevicePairingDialogue::open();
                    }
                });
        }
    }

    juce::ListBox *
    DeviceSelectorComponent::getMidiInputSelectorListBox() const noexcept
    {
        return midiInputsList.get();
    }

} // namespace vmpc_juce::standalone
