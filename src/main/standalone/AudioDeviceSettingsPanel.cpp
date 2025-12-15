#include "standalone/AudioDeviceSettingsPanel.hpp"

#include "standalone/AudioDeviceManager.hpp"
#include "standalone/DeviceSelectorComponent.hpp"
#include "standalone/Utils.hpp"

#include "standalone/ChannelSelectorListBox.hpp"
#include "standalone/AudioDeviceSetupDetails.hpp"
#include "standalone/InputLevelMeter.hpp"

using namespace vmpc_juce::standalone;

AudioDeviceSettingsPanel::AudioDeviceSettingsPanel(
    juce::AudioIODeviceType &t, const AudioDeviceSetupDetails &setupDetails,
    DeviceSelectorComponent &p, const juce::Font &mainFontToUse)
    : mainFont(mainFontToUse), type(t), setup(setupDetails), parent(p)
{
    type.scanForDevices();

    setup.manager->addChangeListener(this);

    {
        outputDeviceDropDown = std::make_unique<juce::ComboBox>();
        outputDeviceDropDown->onChange = [this]
        {
            updateConfig(true, false, false, false);
        };

        addAndMakeVisible(outputDeviceDropDown.get());

        outputDeviceLabel = std::make_unique<juce::Label>(
            juce::String{},
            type.hasSeparateInputsAndOutputs() ? "Output Device" : "Device");
        outputDeviceLabel->setJustificationType(
            juce::Justification::centredLeft);
        addAndMakeVisible(outputDeviceLabel.get());
    }

    {
        inputDeviceDropDown = std::make_unique<juce::ComboBox>();
        inputDeviceDropDown->onChange = [this]
        {
            updateConfig(false, true, false, false);
        };
        addAndMakeVisible(inputDeviceDropDown.get());

        inputDeviceLabel =
            std::make_unique<juce::Label>(juce::String{}, "Input Device");
        inputDeviceLabel->setJustificationType(
            juce::Justification::centredLeft);
        addAndMakeVisible(inputDeviceLabel.get());

        inputLevelMeter = std::make_unique<InputLevelMeter>(*setup.manager);
        addAndMakeVisible(inputLevelMeter.get());
    }

    {
        testButton =
            std::make_unique<juce::TextButton>("Test", "Plays a test tone");
        addAndMakeVisible(testButton.get());
        testButton->onClick = [this]
        {
            playTestSound();
        };
    }

    {
        stereoOutList = std::make_unique<ChannelSelectorListBox>(
            true, setup, ChannelSelectorListBox::audioOutputType, 0, 2,
            mainFont);

        addAndMakeVisible(stereoOutList.get());

        stereoOutLabel =
            std::make_unique<juce::Label>(juce::String{}, "STEREO OUT L/R");

        stereoOutLabel->setJustificationType(juce::Justification::centred);

        addAndMakeVisible(stereoOutLabel.get());
    }

    {
        std::vector<std::string> suffixes{"MIX 1", "MIX 2", "MIX 3", "MIX 4",
                                          "MIX 5", "MIX 6", "MIX 7", "MIX 8"};

        assignableMixOutList = std::make_unique<ChannelSelectorListBox>(
            false, setup, ChannelSelectorListBox::audioOutputType, 2, 8,
            mainFont,
            std::move(suffixes));

        addAndMakeVisible(assignableMixOutList.get());

        assignableMixOutLabel = std::make_unique<juce::Label>(
            juce::String{}, "ASSIGNABLE MIX OUT 1 - 8");

        assignableMixOutLabel->setJustificationType(
            juce::Justification::centred);

        addAndMakeVisible(assignableMixOutLabel.get());
    }

    {
        recordInDropDown = std::make_unique<juce::ComboBox>();

        addAndMakeVisible(recordInDropDown.get());
        recordInLabel =
            std::make_unique<juce::Label>(juce::String{}, "RECORD IN L/R");
        recordInLabel->setJustificationType(juce::Justification::centredLeft);

        addAndMakeVisible(recordInLabel.get());
    }

    {
        sampleRateDropDown = std::make_unique<juce::ComboBox>();
        addAndMakeVisible(sampleRateDropDown.get());

        sampleRateLabel =
            std::make_unique<juce::Label>(juce::String{}, "Sample Rate");

        sampleRateLabel->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(sampleRateLabel.get());
    }

    {
        bufferSizeDropDown = std::make_unique<juce::ComboBox>();
        addAndMakeVisible(bufferSizeDropDown.get());

        bufferSizeLabel =
            std::make_unique<juce::Label>(juce::String{}, "Audio Buffer Size");

        bufferSizeLabel->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(bufferSizeLabel.get());
    }

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

    constexpr int maxListBoxHeight = 90;
    const int h = parent.getItemHeight();
    const int space = h / 4;

    {
        auto row = r.removeFromTop(h);

        testButton->changeWidthToFitText(h);
        testButton->setBounds(row.removeFromRight(testButton->getWidth()));
        row.removeFromRight(space);

        outputDeviceDropDown->setBounds(row);
        r.removeFromTop(space);
    }

    {
        auto row = r.removeFromTop(h);

        inputLevelMeter->setBounds(row.removeFromRight(testButton->getWidth()));
        row.removeFromRight(space);
        inputDeviceDropDown->setBounds(row);
        r.removeFromTop(space);
    }

    {
        stereoOutList->setRowHeight(juce::jmin(22, h));

        const auto listHeight = stereoOutList->getBestHeight(maxListBoxHeight);

        const auto labelArea = r.removeFromTop(h);
        const auto listArea = r.removeFromTop(listHeight);

        stereoOutLabel->setBounds(listArea.getX(), labelArea.getY(),
                                  listArea.getWidth(), h);

        stereoOutList->setBounds(listArea);

        r.removeFromTop(space);
    }

    {
        assignableMixOutList->setRowHeight(juce::jmin(22, h));

        constexpr auto listHeight = maxListBoxHeight;

        const auto labelArea = r.removeFromTop(h);
        const auto listArea = r.removeFromTop(listHeight);

        assignableMixOutLabel->setBounds(listArea.getX(), labelArea.getY(),
                                         listArea.getWidth(), h);

        assignableMixOutList->setBounds(listArea);

        r.removeFromTop(space);
    }

    {
        recordInDropDown->setVisible(true);
        recordInDropDown->setBounds(r.removeFromTop(h));
        r.removeFromTop(space);
    }

    r.removeFromTop(space * 2);

    {
        sampleRateDropDown->setVisible(true);
        sampleRateDropDown->setBounds(r.removeFromTop(h));
        r.removeFromTop(space);
    }

    {
        bufferSizeDropDown->setVisible(true);

        bufferSizeDropDown->setBounds(r.removeFromTop(h));
        r.removeFromTop(space);
    }

    r.removeFromTop(space);

    if (showUIButton != nullptr || resetDeviceButton != nullptr)
    {
        auto buttons = r.removeFromTop(h);

        if (showUIButton != nullptr)
        {
            showUIButton->setVisible(true);
            showUIButton->changeWidthToFitText(h);
            showUIButton->setBounds(
                buttons.removeFromLeft(showUIButton->getWidth()));
            buttons.removeFromLeft(space);
        }

        if (resetDeviceButton != nullptr)
        {
            resetDeviceButton->setVisible(true);
            resetDeviceButton->changeWidthToFitText(h);
            resetDeviceButton->setBounds(
                buttons.removeFromLeft(resetDeviceButton->getWidth()));
        }

        r.removeFromTop(space);
    }

    constexpr int labelAreaWidth = 150;
    constexpr int labelAreaLeftMargin = 10;

    outputDeviceLabel->setBounds(labelAreaLeftMargin,
                                 outputDeviceDropDown->getY(), labelAreaWidth,
                                 outputDeviceDropDown->getHeight());
    inputDeviceLabel->setBounds(labelAreaLeftMargin,
                                inputDeviceDropDown->getY(), labelAreaWidth,
                                inputDeviceDropDown->getHeight());
    sampleRateLabel->setBounds(labelAreaLeftMargin, sampleRateDropDown->getY(),
                               labelAreaWidth, sampleRateDropDown->getHeight());
    bufferSizeLabel->setBounds(labelAreaLeftMargin, bufferSizeDropDown->getY(),
                               labelAreaWidth, bufferSizeDropDown->getHeight());

    recordInLabel->setBounds(labelAreaLeftMargin, recordInDropDown->getY(),
                             labelAreaWidth, recordInDropDown->getHeight());

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
        config.outputDeviceName = outputDeviceDropDown->getSelectedId() < 0
                                      ? juce::String()
                                      : outputDeviceDropDown->getText();

        config.inputDeviceName = inputDeviceDropDown->getSelectedId() < 0
                                     ? juce::String()
                                     : inputDeviceDropDown->getText();

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

    auto *currentDevice = setup.manager->getCurrentAudioDevice();

    stereoOutList->refresh();
    assignableMixOutList->refresh();

    updateRecordInComboBox(currentDevice);
    updateSampleRateComboBox(currentDevice);
    updateBufferSizeComboBox(currentDevice);

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

    testButton->setEnabled(findSelectedDeviceIndex(isInput) >= 0);
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

void AudioDeviceSettingsPanel::updateOutputsComboBox() const
{
    if (setup.maxNumOutputChannels > 0 || !type.hasSeparateInputsAndOutputs())
    {
        addNamesToDeviceBox(*outputDeviceDropDown, false);
    }

    updateSelectedOutput();
}

void AudioDeviceSettingsPanel::updateInputsComboBox() const
{
    if (setup.maxNumInputChannels > 0 && type.hasSeparateInputsAndOutputs())
    {
        addNamesToDeviceBox(*inputDeviceDropDown, true);
    }

    updateSelectedInput();
}

void AudioDeviceSettingsPanel::updateRecordInComboBox(
    juce::AudioIODevice *currentDevice) const
{
    recordInDropDown->clear();
    recordInDropDown->onChange = nullptr;

    if (!currentDevice)
    {
        return;
    }

    const auto chNames = currentDevice->getInputChannelNames();

    juce::StringArray pairs;

    for (int i = 0; i < chNames.size(); i += 2)
    {
        auto &name = chNames[i];

        if (i + 1 >= chNames.size())
        {
            pairs.add(name.trim());
        }
        else
        {
            pairs.add(Utils::getNameForChannelPair(name, chNames[i + 1]));
        }
    }

    for (auto chName : pairs)
    {
        recordInDropDown->addItem(chName, chName.hashCode());
    }

    const auto selectedInputChannels = currentDevice->getActiveInputChannels();

    for (int i = 0; i < chNames.size(); ++i)
    {
        if (selectedInputChannels[i])
        {
            recordInDropDown->setText(pairs[i / 2], juce::dontSendNotification);
            break;
        }
    }

    recordInDropDown->onChange = [this, inputChCount = chNames.size()]
    {
        auto config = setup.manager->getAudioDeviceSetup();
        config.useDefaultInputChannels = false;
        config.inputChannels.clear();
        config.inputChannels.setBit(recordInDropDown->getSelectedItemIndex() *
                                    2);

        if (recordInDropDown->getSelectedItemIndex() * 2 + 1 < inputChCount)
        {
            config.inputChannels.setBit(
                recordInDropDown->getSelectedItemIndex() * 2 + 1);
        }

        setup.manager->setAudioDeviceSetup(config, true);
    };
}

void AudioDeviceSettingsPanel::updateSampleRateComboBox(
    juce::AudioIODevice *currentDevice)
{
    sampleRateDropDown->clear();
    sampleRateDropDown->onChange = nullptr;

    if (!currentDevice)
    {
        return;
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
    bufferSizeDropDown->clear();
    bufferSizeDropDown->onChange = nullptr;

    if (!currentDevice)
    {
        return;
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
