#include "standalone/AudioDeviceSettingsPanel.hpp"

#include "standalone/DeviceSelectorComponent.hpp"
#include "standalone/Utils.hpp"

#include "standalone/ChannelSelectorListBox.hpp"
#include "standalone/AudioDeviceSetupDetails.hpp"
#include "standalone/InputLevelMeter.hpp"

using namespace vmpc_juce::standalone;

AudioDeviceSettingsPanel::AudioDeviceSettingsPanel(
    juce::AudioIODeviceType &t, const AudioDeviceSetupDetails &setupDetails,
    const bool hideAdvancedOptionsWithButton, DeviceSelectorComponent &p)
    : type(t), setup(setupDetails), parent(p)
{
    if (hideAdvancedOptionsWithButton)
    {
        showAdvancedSettingsButton =
            std::make_unique<juce::TextButton>("Show advanced settings...");
        addAndMakeVisible(showAdvancedSettingsButton.get());
        showAdvancedSettingsButton->setClickingTogglesState(true);
        showAdvancedSettingsButton->onClick = [this]
        {
            toggleAdvancedSettings();
        };
    }

    type.scanForDevices();

    setup.manager->addChangeListener(this);

    updateAllControls();
}

AudioDeviceSettingsPanel::~AudioDeviceSettingsPanel()
{
    setup.manager->removeChangeListener(this);
}

void AudioDeviceSettingsPanel::resized()
{
    juce::Rectangle r(proportionOfWidth(0.35f), 0, proportionOfWidth(0.6f),
                      3000);

    constexpr int maxListBoxHeight = 100;
    const int h = parent.getItemHeight();
    const int space = h / 4;

    if (outputDeviceDropDown != nullptr)
    {
        auto row = r.removeFromTop(h);

        if (testButton != nullptr)
        {
            testButton->changeWidthToFitText(h);
            testButton->setBounds(row.removeFromRight(testButton->getWidth()));
            row.removeFromRight(space);
        }

        outputDeviceDropDown->setBounds(row);
        r.removeFromTop(space);
    }

    if (inputDeviceDropDown != nullptr)
    {
        auto row = r.removeFromTop(h);

        inputLevelMeter->setBounds(
            row.removeFromRight(testButton != nullptr ? testButton->getWidth()
                                                      : row.getWidth() / 6));
        row.removeFromRight(space);
        inputDeviceDropDown->setBounds(row);
        r.removeFromTop(space);
    }

    if (outputChanList != nullptr)
    {
        outputChanList->setRowHeight(juce::jmin(22, h));
        outputChanList->setBounds(
            r.removeFromTop(outputChanList->getBestHeight(maxListBoxHeight)));
        outputChanLabel->setBounds(
            0, outputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
        r.removeFromTop(space);
    }

    if (inputChanList != nullptr)
    {
        inputChanList->setRowHeight(juce::jmin(22, h));
        inputChanList->setBounds(
            r.removeFromTop(inputChanList->getBestHeight(maxListBoxHeight)));
        inputChanLabel->setBounds(
            0, inputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
        r.removeFromTop(space);
    }

    r.removeFromTop(space * 2);

    if (showAdvancedSettingsButton != nullptr &&
        sampleRateDropDown != nullptr && bufferSizeDropDown != nullptr)
    {
        showAdvancedSettingsButton->setBounds(r.removeFromTop(h));
        r.removeFromTop(space);
        showAdvancedSettingsButton->changeWidthToFitText();
    }

    const auto advancedSettingsVisible =
        showAdvancedSettingsButton == nullptr ||
        showAdvancedSettingsButton->getToggleState();

    if (sampleRateDropDown != nullptr)
    {
        sampleRateDropDown->setVisible(advancedSettingsVisible);

        if (advancedSettingsVisible)
        {
            sampleRateDropDown->setBounds(r.removeFromTop(h));
            r.removeFromTop(space);
        }
    }

    if (bufferSizeDropDown != nullptr)
    {
        bufferSizeDropDown->setVisible(advancedSettingsVisible);

        if (advancedSettingsVisible)
        {
            bufferSizeDropDown->setBounds(r.removeFromTop(h));
            r.removeFromTop(space);
        }
    }

    r.removeFromTop(space);

    if (showUIButton != nullptr || resetDeviceButton != nullptr)
    {
        auto buttons = r.removeFromTop(h);

        if (showUIButton != nullptr)
        {
            showUIButton->setVisible(advancedSettingsVisible);
            showUIButton->changeWidthToFitText(h);
            showUIButton->setBounds(
                buttons.removeFromLeft(showUIButton->getWidth()));
            buttons.removeFromLeft(space);
        }

        if (resetDeviceButton != nullptr)
        {
            resetDeviceButton->setVisible(advancedSettingsVisible);
            resetDeviceButton->changeWidthToFitText(h);
            resetDeviceButton->setBounds(
                buttons.removeFromLeft(resetDeviceButton->getWidth()));
        }

        r.removeFromTop(space);
    }

    setSize(getWidth(), r.getY());
}

void AudioDeviceSettingsPanel::updateConfig(const bool updateOutputDevice,
                                            const bool updateInputDevice,
                                            const bool updateSampleRate,
                                            const bool updateBufferSize)
{
    auto config = setup.manager->getAudioDeviceSetup();
    juce::String error;

    if (updateOutputDevice || updateInputDevice)
    {
        if (outputDeviceDropDown != nullptr)
        {
            config.outputDeviceName = outputDeviceDropDown->getSelectedId() < 0
                                          ? juce::String()
                                          : outputDeviceDropDown->getText();
        }

        if (inputDeviceDropDown != nullptr)
        {
            config.inputDeviceName = inputDeviceDropDown->getSelectedId() < 0
                                         ? juce::String()
                                         : inputDeviceDropDown->getText();
        }

        if (!type.hasSeparateInputsAndOutputs())
        {
            config.inputDeviceName = config.outputDeviceName;
        }

        if (updateInputDevice)
        {
            config.useDefaultInputChannels = true;
        }
        else
        {
            config.useDefaultOutputChannels = true;
        }

        error = setup.manager->setAudioDeviceSetup(config, true);

        updateSelectedInput();
        updateSelectedOutput();
        updateControlPanelButton();
        resized();
    }
    else if (updateSampleRate)
    {
        if (sampleRateDropDown->getSelectedId() > 0)
        {
            config.sampleRate = sampleRateDropDown->getSelectedId();
            error = setup.manager->setAudioDeviceSetup(config, true);
        }
    }
    else if (updateBufferSize)
    {
        if (bufferSizeDropDown->getSelectedId() > 0)
        {
            config.bufferSize = bufferSizeDropDown->getSelectedId();
            error = setup.manager->setAudioDeviceSetup(config, true);
        }
    }

    if (error.isNotEmpty())
    {
        messageBox = juce::AlertWindow::showScopedAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::WarningIcon)
                .withTitle("Error when trying to open audio device!")
                .withMessage(error)
                .withButton("OK"),
            nullptr);
    }
}

bool AudioDeviceSettingsPanel::showDeviceControlPanel() const
{
    if (auto *device = setup.manager->getCurrentAudioDevice())
    {
        Component modalWindow;
        modalWindow.setOpaque(true);
        modalWindow.addToDesktop(0);
        modalWindow.enterModalState();

        return device->showControlPanel();
    }

    return false;
}

void AudioDeviceSettingsPanel::toggleAdvancedSettings()
{
    showAdvancedSettingsButton->setButtonText(
        (showAdvancedSettingsButton->getToggleState() ? "Hide " : "Show ") +
        juce::String("advanced settings..."));
    resized();
}

void AudioDeviceSettingsPanel::showDeviceUIPanel() const
{
    if (showDeviceControlPanel())
    {
        setup.manager->closeAudioDevice();
        setup.manager->restartLastAudioDevice();
        getTopLevelComponent()->toFront(true);
    }
}

void AudioDeviceSettingsPanel::playTestSound() const
{
    setup.manager->playTestSound();
}

void AudioDeviceSettingsPanel::updateAllControls()
{
    updateOutputsComboBox();
    updateInputsComboBox();

    updateControlPanelButton();
    updateResetButton();

    if (auto *currentDevice = setup.manager->getCurrentAudioDevice())
    {
        if (setup.maxNumOutputChannels > 0 &&
            setup.minNumOutputChannels < setup.manager->getCurrentAudioDevice()
                                             ->getOutputChannelNames()
                                             .size())
        {
            if (outputChanList == nullptr)
            {
                outputChanList = std::make_unique<ChannelSelectorListBox>(
                    setup, ChannelSelectorListBox::audioOutputType,
                    "(no audio output channels found)");
                addAndMakeVisible(outputChanList.get());
                outputChanLabel = std::make_unique<juce::Label>(
                    juce::String{}, "Active output channels:");
                outputChanLabel->setJustificationType(
                    juce::Justification::centredRight);
                outputChanLabel->attachToComponent(outputChanList.get(), true);
            }

            outputChanList->refresh();
        }
        else
        {
            outputChanLabel.reset();
            outputChanList.reset();
        }

        if (setup.maxNumInputChannels > 0 &&
            setup.minNumInputChannels < setup.manager->getCurrentAudioDevice()
                                            ->getInputChannelNames()
                                            .size())
        {
            if (inputChanList == nullptr)
            {
                inputChanList = std::make_unique<ChannelSelectorListBox>(
                    setup, ChannelSelectorListBox::audioInputType,
                    "(no audio input channels found)");
                addAndMakeVisible(inputChanList.get());
                inputChanLabel = std::make_unique<juce::Label>(
                    juce::String{}, "Active input channels:");
                inputChanLabel->setJustificationType(
                    juce::Justification::centredRight);
                inputChanLabel->attachToComponent(inputChanList.get(), true);
            }

            inputChanList->refresh();
        }
        else
        {
            inputChanLabel.reset();
            inputChanList.reset();
        }

        updateSampleRateComboBox(currentDevice);
        updateBufferSizeComboBox(currentDevice);
    }
    else
    {
        jassert(setup.manager->getCurrentAudioDevice() ==
                nullptr); // not the correct device type!

        inputChanLabel.reset();
        outputChanLabel.reset();
        sampleRateLabel.reset();
        bufferSizeLabel.reset();

        inputChanList.reset();
        outputChanList.reset();
        sampleRateDropDown.reset();
        bufferSizeDropDown.reset();

        if (outputDeviceDropDown != nullptr)
        {
            outputDeviceDropDown->setSelectedId(-1, juce::dontSendNotification);
        }

        if (inputDeviceDropDown != nullptr)
        {
            inputDeviceDropDown->setSelectedId(-1, juce::dontSendNotification);
        }
    }

    sendLookAndFeelChange();
    resized();
    setSize(getWidth(), getLowestY() + 4);
}

void AudioDeviceSettingsPanel::changeListenerCallback(juce::ChangeBroadcaster *)
{
    updateAllControls();
}

void AudioDeviceSettingsPanel::resetDevice() const
{
    setup.manager->closeAudioDevice();
    setup.manager->restartLastAudioDevice();
}

int AudioDeviceSettingsPanel::findSelectedDeviceIndex(const bool isInput) const
{
    const auto device = setup.manager->getAudioDeviceSetup();
    const auto deviceName =
        isInput ? device.inputDeviceName : device.outputDeviceName;
    return type.getDeviceNames(isInput).indexOf(deviceName);
}

void AudioDeviceSettingsPanel::updateSelectedInput() const
{
    showCorrectDeviceName(inputDeviceDropDown.get(), true);
}

void AudioDeviceSettingsPanel::updateSelectedOutput() const
{
    constexpr auto isInput = false;
    showCorrectDeviceName(outputDeviceDropDown.get(), isInput);

    if (testButton != nullptr)
    {
        testButton->setEnabled(findSelectedDeviceIndex(isInput) >= 0);
    }
}

void AudioDeviceSettingsPanel::showCorrectDeviceName(juce::ComboBox *box,
                                                     const bool isInput) const
{
    if (box == nullptr)
    {
        return;
    }

    const auto index = findSelectedDeviceIndex(isInput);
    box->setSelectedId(index < 0 ? index : index + 1,
                       juce::dontSendNotification);
}

void AudioDeviceSettingsPanel::addNamesToDeviceBox(juce::ComboBox &combo,
                                                   const bool isInputs) const
{
    const juce::StringArray devs(type.getDeviceNames(isInputs));

    combo.clear(juce::dontSendNotification);

    for (int i = 0; i < devs.size(); ++i)
    {
        combo.addItem(devs[i], i + 1);
    }

    combo.addItem(Utils::getNoDeviceString(), -1);
    combo.setSelectedId(-1, juce::dontSendNotification);
}

int AudioDeviceSettingsPanel::getLowestY() const
{
    int y = 0;

    for (const auto *c : getChildren())
    {
        y = juce::jmax(y, c->getBottom());
    }

    return y;
}

void AudioDeviceSettingsPanel::updateControlPanelButton()
{
    const auto *currentDevice = setup.manager->getCurrentAudioDevice();
    showUIButton.reset();

    if (currentDevice != nullptr && currentDevice->hasControlPanel())
    {
        showUIButton = std::make_unique<juce::TextButton>(
            "Control Panel", "Opens the device's own control panel");
        addAndMakeVisible(showUIButton.get());
        showUIButton->onClick = [this]
        {
            showDeviceUIPanel();
        };
    }

    resized();
}

void AudioDeviceSettingsPanel::updateResetButton()
{
    if (const auto *currentDevice = setup.manager->getCurrentAudioDevice())
    {
        if (currentDevice->hasControlPanel())
        {
            if (resetDeviceButton == nullptr)
            {
                resetDeviceButton = std::make_unique<juce::TextButton>(
                    "Reset Device",
                    "Resets the audio interface - sometimes "
                    "needed after changing a device's properties "
                    "in its custom control panel");
                addAndMakeVisible(resetDeviceButton.get());
                resetDeviceButton->onClick = [this]
                {
                    resetDevice();
                };
                resized();
            }

            return;
        }
    }

    resetDeviceButton.reset();
}

void AudioDeviceSettingsPanel::updateOutputsComboBox()
{
    if (setup.maxNumOutputChannels > 0 || !type.hasSeparateInputsAndOutputs())
    {
        if (outputDeviceDropDown == nullptr)
        {
            outputDeviceDropDown = std::make_unique<juce::ComboBox>();
            outputDeviceDropDown->onChange = [this]
            {
                updateConfig(true, false, false, false);
            };

            addAndMakeVisible(outputDeviceDropDown.get());

            outputDeviceLabel = std::make_unique<juce::Label>(
                juce::String{},
                type.hasSeparateInputsAndOutputs() ? "Output:" : "Device:");
            outputDeviceLabel->attachToComponent(outputDeviceDropDown.get(),
                                                 true);

            if (setup.maxNumOutputChannels > 0)
            {
                testButton = std::make_unique<juce::TextButton>(
                    "Test", "Plays a test tone");
                addAndMakeVisible(testButton.get());
                testButton->onClick = [this]
                {
                    playTestSound();
                };
            }
        }

        addNamesToDeviceBox(*outputDeviceDropDown, false);
    }

    updateSelectedOutput();
}

void AudioDeviceSettingsPanel::updateInputsComboBox()
{
    if (setup.maxNumInputChannels > 0 && type.hasSeparateInputsAndOutputs())
    {
        if (inputDeviceDropDown == nullptr)
        {
            inputDeviceDropDown = std::make_unique<juce::ComboBox>();
            inputDeviceDropDown->onChange = [this]
            {
                updateConfig(false, true, false, false);
            };
            addAndMakeVisible(inputDeviceDropDown.get());

            inputDeviceLabel =
                std::make_unique<juce::Label>(juce::String{}, "Input:");
            inputDeviceLabel->attachToComponent(inputDeviceDropDown.get(),
                                                true);

            inputLevelMeter = std::make_unique<InputLevelMeter>(*setup.manager);
            addAndMakeVisible(inputLevelMeter.get());
        }

        addNamesToDeviceBox(*inputDeviceDropDown, true);
    }

    updateSelectedInput();
}

void AudioDeviceSettingsPanel::updateSampleRateComboBox(
    juce::AudioIODevice *currentDevice)
{
    if (sampleRateDropDown == nullptr)
    {
        sampleRateDropDown = std::make_unique<juce::ComboBox>();
        addAndMakeVisible(sampleRateDropDown.get());

        sampleRateLabel =
            std::make_unique<juce::Label>(juce::String{}, "Sample rate:");
        sampleRateLabel->attachToComponent(sampleRateDropDown.get(), true);
    }
    else
    {
        sampleRateDropDown->clear();
        sampleRateDropDown->onChange = nullptr;
    }

    const auto getFrequencyString = [](const int rate)
    {
        return juce::String(rate) + " Hz";
    };

    for (const auto rate : currentDevice->getAvailableSampleRates())
    {
        const auto intRate = juce::roundToInt(rate);
        sampleRateDropDown->addItem(getFrequencyString(intRate), intRate);
    }

    const auto intRate =
        juce::roundToInt(currentDevice->getCurrentSampleRate());
    sampleRateDropDown->setText(getFrequencyString(intRate),
                                juce::dontSendNotification);

    sampleRateDropDown->onChange = [this]
    {
        updateConfig(false, false, true, false);
    };
}

void AudioDeviceSettingsPanel::updateBufferSizeComboBox(
    juce::AudioIODevice *currentDevice)
{
    if (bufferSizeDropDown == nullptr)
    {
        bufferSizeDropDown = std::make_unique<juce::ComboBox>();
        addAndMakeVisible(bufferSizeDropDown.get());

        bufferSizeLabel =
            std::make_unique<juce::Label>(juce::String{}, "Audio buffer size:");
        bufferSizeLabel->attachToComponent(bufferSizeDropDown.get(), true);
    }
    else
    {
        bufferSizeDropDown->clear();
        bufferSizeDropDown->onChange = nullptr;
    }

    auto currentRate = currentDevice->getCurrentSampleRate();

    if (juce::exactlyEqual(currentRate, 0.0))
    {
        currentRate = 48000.0;
    }

    for (const auto bs : currentDevice->getAvailableBufferSizes())
    {
        bufferSizeDropDown->addItem(
            juce::String(bs) + " samples (" +
                juce::String(bs * 1000.0 / currentRate, 1) + " ms)",
            bs);
    }

    bufferSizeDropDown->setSelectedId(
        currentDevice->getCurrentBufferSizeSamples(),
        juce::dontSendNotification);
    bufferSizeDropDown->onChange = [this]
    {
        updateConfig(false, false, false, true);
    };
}
