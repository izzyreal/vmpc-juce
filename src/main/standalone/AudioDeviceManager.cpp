#include "standalone/AudioDeviceManager.hpp"

using namespace vmpc_juce::standalone;

template <typename Setup> static auto getSetupInfo(Setup &s, bool isInput)
{
    struct SetupInfo
    {
        decltype((s.inputDeviceName)) name;
        decltype((s.inputChannels)) channels;
        decltype((s.useDefaultInputChannels)) useDefault;
    };

    return isInput ? SetupInfo{s.inputDeviceName, s.inputChannels,
                               s.useDefaultInputChannels}
                   : SetupInfo{s.outputDeviceName, s.outputChannels,
                               s.useDefaultOutputChannels};
}

static auto tie(const AudioDeviceManager::AudioDeviceSetup &s)
{
    return std::tie(s.outputDeviceName, s.inputDeviceName, s.sampleRate,
                    s.bufferSize, s.inputChannels, s.useDefaultInputChannels,
                    s.outputChannels, s.useDefaultOutputChannels);
}

bool AudioDeviceManager::AudioDeviceSetup::operator==(
    const AudioDeviceSetup &other) const
{
    return tie(*this) == tie(other);
}

bool AudioDeviceManager::AudioDeviceSetup::operator!=(
    const AudioDeviceSetup &other) const
{
    return tie(*this) != tie(other);
}

class AudioDeviceManager::CallbackHandler final
    : public juce::AudioIODeviceCallback,
      public juce::MidiInputCallback,
      public juce::AudioIODeviceType::Listener
{
public:
    explicit CallbackHandler(AudioDeviceManager &adm) noexcept : owner(adm) {}

private:
    void audioDeviceIOCallbackWithContext(
        const float *const *ins, const int numIns, float *const *outs,
        const int numOuts, const int numSamples,
        const juce::AudioIODeviceCallbackContext &context) override
    {
        owner.audioDeviceIOCallbackInt(ins, numIns, outs, numOuts, numSamples,
                                       context);
    }

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override
    {
        owner.audioDeviceAboutToStartInt(device);
    }

    void audioDeviceStopped() override
    {
        owner.audioDeviceStoppedInt();
    }

    void audioDeviceError(const juce::String &message) override
    {
        owner.audioDeviceErrorInt(message);
    }

    void handleIncomingMidiMessage(juce::MidiInput *source,
                                   const juce::MidiMessage &message) override
    {
        owner.handleIncomingMidiMessageInt(source, message);
    }

    void audioDeviceListChanged() override
    {
        owner.audioDeviceListChanged();
    }

    AudioDeviceManager &owner;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CallbackHandler)
};

//==============================================================================
AudioDeviceManager::AudioDeviceManager()
{
    callbackHandler.reset(new CallbackHandler(*this));
}

AudioDeviceManager::~AudioDeviceManager()
{
    currentAudioDevice.reset();
    defaultMidiOutput.reset();
}

//==============================================================================
void AudioDeviceManager::createDeviceTypesIfNeeded()
{
    if (availableDeviceTypes.size() == 0)
    {
        juce::OwnedArray<juce::AudioIODeviceType> types;
        createAudioDeviceTypes(types);

        for (auto *t : types)
        {
            addAudioDeviceType(std::unique_ptr<juce::AudioIODeviceType>(t));
        }

        types.clear(false);

        for (auto *type : availableDeviceTypes)
        {
            type->scanForDevices();
        }

        pickCurrentDeviceTypeWithDevices();
    }
}

void AudioDeviceManager::pickCurrentDeviceTypeWithDevices()
{
    const auto deviceTypeHasDevices = [](const juce::AudioIODeviceType *ptr)
    {
        return !ptr->getDeviceNames(true).isEmpty() ||
               !ptr->getDeviceNames(false).isEmpty();
    };

    if (const auto *type = findType(currentDeviceType))
    {
        if (deviceTypeHasDevices(type))
        {
            return;
        }
    }

    const auto iter =
        std::find_if(availableDeviceTypes.begin(), availableDeviceTypes.end(),
                     deviceTypeHasDevices);

    if (iter != availableDeviceTypes.end())
    {
        currentDeviceType = (*iter)->getTypeName();
    }
}

const juce::OwnedArray<juce::AudioIODeviceType> &
AudioDeviceManager::getAvailableDeviceTypes()
{
    scanDevicesIfNeeded();
    return availableDeviceTypes;
}

void AudioDeviceManager::updateCurrentSetup()
{
    if (currentAudioDevice != nullptr)
    {
        currentSetup.sampleRate = currentAudioDevice->getCurrentSampleRate();
        currentSetup.bufferSize =
            currentAudioDevice->getCurrentBufferSizeSamples();
        currentSetup.inputChannels =
            currentAudioDevice->getActiveInputChannels();
        currentSetup.outputChannels =
            currentAudioDevice->getActiveOutputChannels();
    }
}

void AudioDeviceManager::audioDeviceListChanged()
{
    if (currentAudioDevice != nullptr)
    {
        const auto currentDeviceStillAvailable = [&]
        {
            const auto currentTypeName = currentAudioDevice->getTypeName();
            const auto currentDeviceName = currentAudioDevice->getName();

            for (const auto *deviceType : availableDeviceTypes)
            {
                if (currentTypeName == deviceType->getTypeName())
                {
                    for (auto &deviceName : deviceType->getDeviceNames(true))
                    {
                        if (currentDeviceName == deviceName)
                        {
                            return true;
                        }
                    }

                    for (auto &deviceName : deviceType->getDeviceNames(false))
                    {
                        if (currentDeviceName == deviceName)
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }();

        if (!currentDeviceStillAvailable)
        {
            closeAudioDevice();

            if (const auto e = createStateXml())
            {
                initialiseFromXML(*e, true, preferredDeviceName, &currentSetup);
            }
            else
            {
                initialiseDefault(preferredDeviceName, &currentSetup);
            }
        }

        updateCurrentSetup();
    }

    sendChangeMessage();
}

void AudioDeviceManager::midiDeviceListChanged()
{
    openLastRequestedMidiDevices(midiDeviceInfosFromXml,
                                 defaultMidiOutputDeviceInfo);
    sendChangeMessage();
}

//==============================================================================
static void addIfNotNull(juce::OwnedArray<juce::AudioIODeviceType> &list,
                         juce::AudioIODeviceType *const device)
{
    if (device != nullptr)
    {
        list.add(device);
    }
}

void AudioDeviceManager::createAudioDeviceTypes(
    juce::OwnedArray<juce::AudioIODeviceType> &list)
{
    addIfNotNull(list, juce::AudioIODeviceType::createAudioIODeviceType_WASAPI(
                           juce::WASAPIDeviceMode::shared));
    addIfNotNull(list, juce::AudioIODeviceType::createAudioIODeviceType_WASAPI(
                           juce::WASAPIDeviceMode::exclusive));
    addIfNotNull(list, juce::AudioIODeviceType::createAudioIODeviceType_WASAPI(
                           juce::WASAPIDeviceMode::sharedLowLatency));
    addIfNotNull(
        list, juce::AudioIODeviceType::createAudioIODeviceType_DirectSound());
    addIfNotNull(list, juce::AudioIODeviceType::createAudioIODeviceType_ASIO());
    addIfNotNull(list,
                 juce::AudioIODeviceType::createAudioIODeviceType_CoreAudio());
    addIfNotNull(list,
                 juce::AudioIODeviceType::createAudioIODeviceType_iOSAudio());
    addIfNotNull(list, juce::AudioIODeviceType::createAudioIODeviceType_Bela());
    addIfNotNull(list, juce::AudioIODeviceType::createAudioIODeviceType_ALSA());
    addIfNotNull(list, juce::AudioIODeviceType::createAudioIODeviceType_JACK());
    addIfNotNull(list, juce::AudioIODeviceType::createAudioIODeviceType_Oboe());
    addIfNotNull(list,
                 juce::AudioIODeviceType::createAudioIODeviceType_OpenSLES());
    addIfNotNull(list,
                 juce::AudioIODeviceType::createAudioIODeviceType_Android());
}

void AudioDeviceManager::addAudioDeviceType(
    std::unique_ptr<juce::AudioIODeviceType> newDeviceType)
{
    if (newDeviceType != nullptr)
    {
        jassert(lastDeviceTypeConfigs.size() == availableDeviceTypes.size());

        availableDeviceTypes.add(newDeviceType.release());
        lastDeviceTypeConfigs.add(new AudioDeviceSetup());

        availableDeviceTypes.getLast()->addListener(callbackHandler.get());
    }
}

void AudioDeviceManager::removeAudioDeviceType(
    const juce::AudioIODeviceType *deviceTypeToRemove)
{
    if (deviceTypeToRemove != nullptr)
    {
        jassert(lastDeviceTypeConfigs.size() == availableDeviceTypes.size());

        const auto index = availableDeviceTypes.indexOf(deviceTypeToRemove);

        if (const auto removed = std::unique_ptr<juce::AudioIODeviceType>(
                availableDeviceTypes.removeAndReturn(index)))
        {
            removed->removeListener(callbackHandler.get());
            lastDeviceTypeConfigs.remove(index, true);
        }
    }
}

static bool deviceListContains(const juce::AudioIODeviceType *type,
                               const bool isInput, const juce::String &name)
{
    for (auto &deviceName : type->getDeviceNames(isInput))
    {
        if (deviceName.trim().equalsIgnoreCase(name.trim()))
        {
            return true;
        }
    }

    return false;
}

//==============================================================================
juce::String AudioDeviceManager::initialise(
    const int numInputChannelsNeeded, const int numOutputChannelsNeeded,
    const juce::XmlElement *const xml, const bool selectDefaultDeviceOnFailure,
    const juce::String &preferredDefaultDeviceName,
    const AudioDeviceSetup *preferredSetupOptions)
{
    scanDevicesIfNeeded();
    pickCurrentDeviceTypeWithDevices();

    numInputChansNeeded = numInputChannelsNeeded;
    numOutputChansNeeded = numOutputChannelsNeeded;
    preferredDeviceName = preferredDefaultDeviceName;

    if (xml != nullptr && xml->hasTagName("DEVICESETUP"))
    {
        return initialiseFromXML(*xml, selectDefaultDeviceOnFailure,
                                 preferredDeviceName, preferredSetupOptions);
    }

    return initialiseDefault(preferredDeviceName, preferredSetupOptions);
}

juce::String AudioDeviceManager::initialiseDefault(
    const juce::String &preferredDefaultDeviceName,
    const AudioDeviceSetup *preferredSetupOptions)
{
    AudioDeviceSetup setup;

    if (preferredSetupOptions != nullptr)
    {
        setup = *preferredSetupOptions;
    }
    else if (preferredDefaultDeviceName.isNotEmpty())
    {
        const auto nameMatches =
            [&preferredDefaultDeviceName](const juce::String &name)
        {
            return name.matchesWildcard(preferredDefaultDeviceName, true);
        };

        struct WildcardMatch
        {
            juce::String value;
            bool successful;
        };

        const auto getWildcardMatch =
            [&nameMatches](const juce::StringArray &names)
        {
            const auto iter =
                std::find_if(names.begin(), names.end(), nameMatches);
            return WildcardMatch{iter != names.end() ? *iter : juce::String(),
                                 iter != names.end()};
        };

        struct WildcardMatches
        {
            WildcardMatch input, output;
        };

        const auto getMatchesForType =
            [&getWildcardMatch](const juce::AudioIODeviceType *type)
        {
            return WildcardMatches{
                getWildcardMatch(type->getDeviceNames(true)),
                getWildcardMatch(type->getDeviceNames(false))};
        };

        struct SearchResult
        {
            juce::String type, input, output;
        };

        const auto result = [&]
        {
            // First, look for a device type with an input and output which
            // matches the preferred name
            for (const auto *type : availableDeviceTypes)
            {
                const auto matches = getMatchesForType(type);

                if (matches.input.successful && matches.output.successful)
                {
                    return SearchResult{type->getTypeName(),
                                        matches.input.value,
                                        matches.output.value};
                }
            }

            // No device type has matching ins and outs, so fall back to a
            // device where either the input or output match
            for (const auto *type : availableDeviceTypes)
            {
                const auto matches = getMatchesForType(type);

                if (matches.input.successful || matches.output.successful)
                {
                    return SearchResult{type->getTypeName(),
                                        matches.input.value,
                                        matches.output.value};
                }
            }

            // No devices match the query, so just use the default devices from
            // the current type
            return SearchResult{currentDeviceType, {}, {}};
        }();

        currentDeviceType = result.type;
        setup.inputDeviceName = result.input;
        setup.outputDeviceName = result.output;
    }

    insertDefaultDeviceNames(setup);
    return setAudioDeviceSetup(setup, false);
}

juce::String AudioDeviceManager::initialiseFromXML(
    const juce::XmlElement &xml, const bool selectDefaultDeviceOnFailure,
    const juce::String &preferredDefaultDeviceName,
    const AudioDeviceSetup *preferredSetupOptions)
{
    lastExplicitSettings.reset(new juce::XmlElement(xml));

    AudioDeviceSetup setup;

    if (preferredSetupOptions != nullptr)
    {
        setup = *preferredSetupOptions;
    }

    if (xml.getStringAttribute("audioDeviceName").isNotEmpty())
    {
        setup.inputDeviceName = setup.outputDeviceName =
            xml.getStringAttribute("audioDeviceName");
    }
    else
    {
        setup.inputDeviceName = xml.getStringAttribute("audioInputDeviceName");
        setup.outputDeviceName =
            xml.getStringAttribute("audioOutputDeviceName");
    }

    currentDeviceType = xml.getStringAttribute("deviceType");

    if (findType(currentDeviceType) == nullptr)
    {
        if (const auto *type =
                findType(setup.inputDeviceName, setup.outputDeviceName))
        {
            currentDeviceType = type->getTypeName();
        }
        else if (const auto *firstType = availableDeviceTypes.getFirst())
        {
            currentDeviceType = firstType->getTypeName();
        }
    }

    setup.bufferSize =
        xml.getIntAttribute("audioDeviceBufferSize", setup.bufferSize);
    setup.sampleRate =
        xml.getDoubleAttribute("audioDeviceRate", setup.sampleRate);

    setup.inputChannels.parseString(
        xml.getStringAttribute("audioDeviceInChans", "11"), 2);
    setup.outputChannels.parseString(
        xml.getStringAttribute("audioDeviceOutChans", "11"), 2);

    setup.useDefaultInputChannels = !xml.hasAttribute("audioDeviceInChans");
    setup.useDefaultOutputChannels = !xml.hasAttribute("audioDeviceOutChans");

    juce::String error = setAudioDeviceSetup(setup, true);

    if (error.isNotEmpty() && selectDefaultDeviceOnFailure)
    {
        error = initialise(numInputChansNeeded, numOutputChansNeeded, nullptr,
                           false, preferredDefaultDeviceName);
    }

    enabledMidiInputs.clear();

    const auto midiInputs = [&]
    {
        juce::Array<juce::MidiDeviceInfo> result;

        for (const auto *c : xml.getChildWithTagNameIterator("MIDIINPUT"))
        {
            result.add({c->getStringAttribute("name"),
                        c->getStringAttribute("identifier")});
        }

        return result;
    }();

    const juce::MidiDeviceInfo defaultOutputDeviceInfo(
        xml.getStringAttribute("defaultMidiOutput"),
        xml.getStringAttribute("defaultMidiOutputDevice"));

    openLastRequestedMidiDevices(midiInputs, defaultOutputDeviceInfo);

    return error;
}

void AudioDeviceManager::openLastRequestedMidiDevices(
    const juce::Array<juce::MidiDeviceInfo> &desiredInputs,
    const juce::MidiDeviceInfo &defaultOutput)
{
    const auto openDeviceIfAvailable =
        [&](const juce::Array<juce::MidiDeviceInfo> &devices,
            const juce::MidiDeviceInfo &deviceToOpen, auto &&doOpen)
    {
        const auto iterWithMatchingIdentifier =
            std::find_if(devices.begin(), devices.end(),
                         [&](const auto &x)
                         {
                             return x.identifier == deviceToOpen.identifier;
                         });

        if (iterWithMatchingIdentifier != devices.end())
        {
            doOpen(deviceToOpen.identifier);
            return;
        }

        const auto iterWithMatchingName =
            std::find_if(devices.begin(), devices.end(),
                         [&](const auto &x)
                         {
                             return x.name == deviceToOpen.name;
                         });

        if (iterWithMatchingName != devices.end())
        {
            doOpen(iterWithMatchingName->identifier);
        }
    };

    midiDeviceInfosFromXml = desiredInputs;

    const auto inputs = juce::MidiInput::getAvailableDevices();

    for (const auto &info : midiDeviceInfosFromXml)
    {
        openDeviceIfAvailable(inputs, info,
                              [&](const auto identifier)
                              {
                                  setMidiInputDeviceEnabled(identifier, true);
                              });
    }

    const auto outputs = juce::MidiOutput::getAvailableDevices();

    openDeviceIfAvailable(outputs, defaultOutput,
                          [&](const auto identifier)
                          {
                              setDefaultMidiOutputDevice(identifier);
                          });
}

juce::String AudioDeviceManager::initialiseWithDefaultDevices(
    const int numInputChannelsNeeded, const int numOutputChannelsNeeded)
{
    lastExplicitSettings.reset();

    return initialise(numInputChannelsNeeded, numOutputChannelsNeeded, nullptr,
                      false, {}, nullptr);
}

void AudioDeviceManager::insertDefaultDeviceNames(AudioDeviceSetup &setup) const
{
    enum class Direction
    {
        out,
        in
    };

    if (auto *type = getCurrentDeviceTypeObject())
    {
        // We avoid selecting a device pair that doesn't share a matching sample
        // rate, if possible. If not, other parts of the AudioDeviceManager and
        // AudioIODevice classes should generate an appropriate error message
        // when opening or starting these devices.
        const auto getDevicesToTestForMatchingSampleRate =
            [&setup, type, this](const Direction dir)
        {
            const auto isInput = dir == Direction::in;
            const auto info = getSetupInfo(setup, isInput);

            if (!info.name.isEmpty())
            {
                return juce::StringArray{info.name};
            }

            const auto numChannelsNeeded =
                isInput ? numInputChansNeeded : numOutputChansNeeded;
            auto deviceNames = numChannelsNeeded > 0
                                   ? type->getDeviceNames(isInput)
                                   : juce::StringArray{};
            deviceNames.move(type->getDefaultDeviceIndex(isInput), 0);

            return deviceNames;
        };

        std::map<std::pair<Direction, juce::String>, juce::Array<double>>
            sampleRatesCache;

        const auto getSupportedSampleRates =
            [&sampleRatesCache, type](Direction dir,
                                      const juce::String &deviceName)
        {
            const auto key = std::make_pair(dir, deviceName);

            auto &entry = [&]() -> auto &
            {
                const auto it = sampleRatesCache.find(key);

                if (it != sampleRatesCache.end())
                {
                    return it->second;
                }

                auto &elem = sampleRatesCache[key];
                const auto tempDevice = rawToUniquePtr(
                    type->createDevice(dir == Direction::in ? "" : deviceName,
                                       dir == Direction::in ? deviceName : ""));
                if (tempDevice != nullptr)
                {
                    elem = tempDevice->getAvailableSampleRates();
                }

                return elem;
            }();

            return entry;
        };

        const auto validate =
            [&getSupportedSampleRates](const juce::String &outputDeviceName,
                                       const juce::String &inputDeviceName)
        {
            jassert(!outputDeviceName.isEmpty() && !inputDeviceName.isEmpty());

            const auto outputSampleRates =
                getSupportedSampleRates(Direction::out, outputDeviceName);
            const auto inputSampleRates =
                getSupportedSampleRates(Direction::in, inputDeviceName);

            return std::any_of(inputSampleRates.begin(), inputSampleRates.end(),
                               [&](auto inputSampleRate)
                               {
                                   return outputSampleRates.contains(
                                       inputSampleRate);
                               });
        };

        auto outputsToTest =
            getDevicesToTestForMatchingSampleRate(Direction::out);
        auto inputsToTest =
            getDevicesToTestForMatchingSampleRate(Direction::in);

        // We set default device names, so in case no in-out pair passes the
        // validation, we still produce the same result as before
        if (setup.outputDeviceName.isEmpty() && !outputsToTest.isEmpty())
        {
            setup.outputDeviceName = outputsToTest[0];
        }

        if (setup.inputDeviceName.isEmpty() && !inputsToTest.isEmpty())
        {
            setup.inputDeviceName = inputsToTest[0];
        }

        // We check all possible in-out pairs until the first validation pass.
        // If no pair passes we leave the setup unchanged.
        for (const auto &out : outputsToTest)
        {
            for (const auto &in : inputsToTest)
            {
                if (validate(out, in))
                {
                    setup.outputDeviceName = out;
                    setup.inputDeviceName = in;

                    return;
                }
            }
        }
    }
}

std::unique_ptr<juce::XmlElement> AudioDeviceManager::createStateXml() const
{
    if (lastExplicitSettings != nullptr)
    {
        return std::make_unique<juce::XmlElement>(*lastExplicitSettings);
    }

    return {};
}

void AudioDeviceManager::scanDevicesIfNeeded()
{
    if (listNeedsScanning)
    {
        listNeedsScanning = false;

        createDeviceTypesIfNeeded();

        for (auto *type : availableDeviceTypes)
        {
            type->scanForDevices();
        }
    }
}

juce::AudioIODeviceType *
AudioDeviceManager::findType(const juce::String &typeName)
{
    scanDevicesIfNeeded();

    for (auto *type : availableDeviceTypes)
    {
        if (type->getTypeName() == typeName)
        {
            return type;
        }
    }

    return {};
}

juce::AudioIODeviceType *
AudioDeviceManager::findType(const juce::String &inputName,
                             const juce::String &outputName)
{
    scanDevicesIfNeeded();

    for (auto *type : availableDeviceTypes)
    {
        if ((inputName.isNotEmpty() &&
             deviceListContains(type, true, inputName)) ||
            (outputName.isNotEmpty() &&
             deviceListContains(type, false, outputName)))
        {
            return type;
        }
    }

    return {};
}

AudioDeviceManager::AudioDeviceSetup
AudioDeviceManager::getAudioDeviceSetup() const
{
    return currentSetup;
}

void AudioDeviceManager::getAudioDeviceSetup(AudioDeviceSetup &setup) const
{
    setup = currentSetup;
}

void AudioDeviceManager::deleteCurrentDevice()
{
    currentAudioDevice.reset();
    currentSetup.inputDeviceName.clear();
    currentSetup.outputDeviceName.clear();
}

void AudioDeviceManager::setCurrentAudioDeviceType(
    const juce::String &type, const bool treatAsChosenDevice)
{
    for (int i = 0; i < availableDeviceTypes.size(); ++i)
    {
        if (availableDeviceTypes.getUnchecked(i)->getTypeName() == type &&
            currentDeviceType != type)
        {
            if (currentAudioDevice != nullptr)
            {
                closeAudioDevice();
                juce::Thread::sleep(
                    1500); // allow a moment for OS devices to sort
                           // themselves out, to help avoid things
                           // like DirectSound/ASIO clashes
            }

            currentDeviceType = type;

            AudioDeviceSetup s(*lastDeviceTypeConfigs.getUnchecked(i));
            insertDefaultDeviceNames(s);

            setAudioDeviceSetup(s, treatAsChosenDevice);

            sendChangeMessage();
            break;
        }
    }
}

juce::AudioWorkgroup AudioDeviceManager::getDeviceAudioWorkgroup() const
{
    return currentAudioDevice != nullptr ? currentAudioDevice->getWorkgroup()
                                         : juce::AudioWorkgroup{};
}

juce::AudioIODeviceType *AudioDeviceManager::getCurrentDeviceTypeObject() const
{
    for (auto *type : availableDeviceTypes)
    {
        if (type->getTypeName() == currentDeviceType)
        {
            return type;
        }
    }

    return availableDeviceTypes.getFirst();
}

static void updateSetupChannels(AudioDeviceManager::AudioDeviceSetup &setup,
                                const int defaultNumIns,
                                const int defaultNumOuts)
{
    auto updateChannels = [](const juce::String &deviceName,
                             juce::BigInteger &channels,
                             const int defaultNumChannels)
    {
        if (deviceName.isEmpty())
        {
            channels.clear();
        }
        else if (defaultNumChannels != -1)
        {
            channels.clear();
            channels.setRange(0, defaultNumChannels, true);
        }
    };

    updateChannels(setup.inputDeviceName, setup.inputChannels,
                   setup.useDefaultInputChannels ? defaultNumIns : -1);
    updateChannels(setup.outputDeviceName, setup.outputChannels,
                   setup.useDefaultOutputChannels ? defaultNumOuts : -1);
}

juce::String
AudioDeviceManager::setAudioDeviceSetup(const AudioDeviceSetup &newSetup,
                                        const bool treatAsChosenDevice)
{
    jassert(&newSetup != &currentSetup); // this will have no effect

    if (newSetup != currentSetup)
    {
        sendChangeMessage();
    }
    else if (currentAudioDevice != nullptr)
    {
        return {};
    }

    stopDevice();

    if (getCurrentDeviceTypeObject() == nullptr ||
        (newSetup.inputDeviceName.isEmpty() &&
         newSetup.outputDeviceName.isEmpty()))
    {
        deleteCurrentDevice();

        if (treatAsChosenDevice)
        {
            updateXml();
        }

        return {};
    }

    juce::String error;

    const auto needsNewDevice =
        currentSetup.inputDeviceName != newSetup.inputDeviceName ||
        currentSetup.outputDeviceName != newSetup.outputDeviceName ||
        currentAudioDevice == nullptr;

    if (needsNewDevice)
    {
        deleteCurrentDevice();
        scanDevicesIfNeeded();

        auto *type = getCurrentDeviceTypeObject();

        for (const auto isInput : {false, true})
        {
            const auto name = getSetupInfo(newSetup, isInput).name;

            if (name.isNotEmpty() && !deviceListContains(type, isInput, name))
            {
                return "No such device: " + name;
            }
        }

        currentAudioDevice.reset(type->createDevice(newSetup.outputDeviceName,
                                                    newSetup.inputDeviceName));

        if (currentAudioDevice == nullptr)
        {
            error =
                "Can't open the audio device!\n\n"
                "This may be because another application is currently using "
                "the same device - "
                "if so, you should close any other applications and try again!";
        }
        else
        {
            error = currentAudioDevice->getLastError();
        }

        if (error.isNotEmpty())
        {
            deleteCurrentDevice();
            return error;
        }
    }

    currentSetup = newSetup;

    if (!currentSetup.useDefaultInputChannels)
    {
        numInputChansNeeded = currentSetup.inputChannels.countNumberOfSetBits();
    }
    if (!currentSetup.useDefaultOutputChannels)
    {
        numOutputChansNeeded =
            currentSetup.outputChannels.countNumberOfSetBits();
    }

    updateSetupChannels(currentSetup, numInputChansNeeded,
                        numOutputChansNeeded);

    if (currentSetup.inputChannels.isZero() &&
        currentSetup.outputChannels.isZero())
    {
        if (treatAsChosenDevice)
        {
            updateXml();
        }

        return {};
    }

    currentSetup.sampleRate = chooseBestSampleRate(currentSetup.sampleRate);
    currentSetup.bufferSize = chooseBestBufferSize(currentSetup.bufferSize);

    error = currentAudioDevice->open(
        currentSetup.inputChannels, currentSetup.outputChannels,
        currentSetup.sampleRate, currentSetup.bufferSize);

    if (error.isEmpty())
    {
        currentDeviceType = currentAudioDevice->getTypeName();

        currentAudioDevice->start(callbackHandler.get());

        error = currentAudioDevice->getLastError();
    }

    if (error.isEmpty())
    {
        updateCurrentSetup();

        for (int i = 0; i < availableDeviceTypes.size(); ++i)
        {
            if (availableDeviceTypes.getUnchecked(i)->getTypeName() ==
                currentDeviceType)
            {
                *lastDeviceTypeConfigs.getUnchecked(i) = currentSetup;
            }
        }

        if (treatAsChosenDevice)
        {
            updateXml();
        }
    }
    else
    {
        deleteCurrentDevice();
    }

    return error;
}

double AudioDeviceManager::chooseBestSampleRate(double rate) const
{
    jassert(currentAudioDevice != nullptr);

    const auto rates = currentAudioDevice->getAvailableSampleRates();

    if (rate > 0 && rates.contains(rate))
    {
        return rate;
    }

    rate = currentAudioDevice->getCurrentSampleRate();

    if (rate > 0 && rates.contains(rate))
    {
        return rate;
    }

    double lowestAbove44 = 0.0;

    for (int i = rates.size(); --i >= 0;)
    {
        const auto sr = rates[i];

        if (sr >= 44100.0 && (lowestAbove44 < 1.0 || sr < lowestAbove44))
        {
            lowestAbove44 = sr;
        }
    }

    if (lowestAbove44 > 0.0)
    {
        return lowestAbove44;
    }

    return rates[0];
}

int AudioDeviceManager::chooseBestBufferSize(const int bufferSize) const
{
    jassert(currentAudioDevice != nullptr);

    if (bufferSize > 0 &&
        currentAudioDevice->getAvailableBufferSizes().contains(bufferSize))
    {
        return bufferSize;
    }

    return currentAudioDevice->getDefaultBufferSize();
}

void AudioDeviceManager::stopDevice()
{
    if (currentAudioDevice != nullptr)
    {
        currentAudioDevice->stop();
    }

    testSound.reset();
}

void AudioDeviceManager::closeAudioDevice()
{
    stopDevice();
    currentAudioDevice.reset();
    loadMeasurer.reset();
}

void AudioDeviceManager::restartLastAudioDevice()
{
    if (currentAudioDevice == nullptr)
    {
        if (currentSetup.inputDeviceName.isEmpty() &&
            currentSetup.outputDeviceName.isEmpty())
        {
            // This method will only reload the last device that was running
            // before closeAudioDevice() was called - you need to actually open
            // one first, with setAudioDeviceSetup().
            jassertfalse;
            return;
        }

        const AudioDeviceSetup s(currentSetup);
        setAudioDeviceSetup(s, false);
    }
}

void AudioDeviceManager::updateXml()
{
    lastExplicitSettings.reset(new juce::XmlElement("DEVICESETUP"));

    lastExplicitSettings->setAttribute("deviceType", currentDeviceType);
    lastExplicitSettings->setAttribute("audioOutputDeviceName",
                                       currentSetup.outputDeviceName);
    lastExplicitSettings->setAttribute("audioInputDeviceName",
                                       currentSetup.inputDeviceName);

    if (currentAudioDevice != nullptr)
    {
        lastExplicitSettings->setAttribute(
            "audioDeviceRate", currentAudioDevice->getCurrentSampleRate());

        if (currentAudioDevice->getDefaultBufferSize() !=
            currentAudioDevice->getCurrentBufferSizeSamples())
        {
            lastExplicitSettings->setAttribute(
                "audioDeviceBufferSize",
                currentAudioDevice->getCurrentBufferSizeSamples());
        }

        if (!currentSetup.useDefaultInputChannels)
        {
            lastExplicitSettings->setAttribute(
                "audioDeviceInChans", currentSetup.inputChannels.toString(2));
        }

        if (!currentSetup.useDefaultOutputChannels)
        {
            lastExplicitSettings->setAttribute(
                "audioDeviceOutChans", currentSetup.outputChannels.toString(2));
        }
    }

    for (const auto &input : enabledMidiInputs)
    {
        auto *child = lastExplicitSettings->createNewChildElement("MIDIINPUT");

        child->setAttribute("name", input->getName());
        child->setAttribute("identifier", input->getIdentifier());
    }

    if (midiDeviceInfosFromXml.size() > 0)
    {
        // Add any midi devices that have been enabled before, but which aren't
        // currently open because the device has been disconnected.
        const auto availableMidiDevices =
            juce::MidiInput::getAvailableDevices();

        for (auto &d : midiDeviceInfosFromXml)
        {
            if (!availableMidiDevices.contains(d))
            {
                auto *child =
                    lastExplicitSettings->createNewChildElement("MIDIINPUT");

                child->setAttribute("name", d.name);
                child->setAttribute("identifier", d.identifier);
            }
        }
    }

    if (defaultMidiOutputDeviceInfo != juce::MidiDeviceInfo())
    {
        lastExplicitSettings->setAttribute("defaultMidiOutput",
                                           defaultMidiOutputDeviceInfo.name);
        lastExplicitSettings->setAttribute(
            "defaultMidiOutputDevice", defaultMidiOutputDeviceInfo.identifier);
    }
}

//==============================================================================
void AudioDeviceManager::addAudioCallback(
    juce::AudioIODeviceCallback *newCallback)
{
    {
        const juce::ScopedLock sl(audioCallbackLock);

        if (callbacks.contains(newCallback))
        {
            return;
        }
    }

    if (currentAudioDevice != nullptr && newCallback != nullptr)
    {
        newCallback->audioDeviceAboutToStart(currentAudioDevice.get());
    }

    const juce::ScopedLock sl(audioCallbackLock);
    callbacks.add(newCallback);
}

void AudioDeviceManager::removeAudioCallback(
    juce::AudioIODeviceCallback *callbackToRemove)
{
    if (callbackToRemove != nullptr)
    {
        bool needsDeinitialising = currentAudioDevice != nullptr;

        {
            const juce::ScopedLock sl(audioCallbackLock);

            needsDeinitialising =
                needsDeinitialising && callbacks.contains(callbackToRemove);
            callbacks.removeFirstMatchingValue(callbackToRemove);
        }

        if (needsDeinitialising)
        {
            callbackToRemove->audioDeviceStopped();
        }
    }
}

void AudioDeviceManager::audioDeviceIOCallbackInt(
    const float *const *inputChannelData, const int numInputChannels,
    float *const *outputChannelData, const int numOutputChannels,
    const int numSamples, const juce::AudioIODeviceCallbackContext &context)
{
    const juce::ScopedLock sl(audioCallbackLock);

    inputLevelGetter->updateLevel(inputChannelData, numInputChannels,
                                  numSamples);

    if (callbacks.size() > 0)
    {
        juce::AudioProcessLoadMeasurer::ScopedTimer timer(loadMeasurer,
                                                          numSamples);

        tempBuffer.setSize(juce::jmax(1, numOutputChannels),
                           juce::jmax(1, numSamples), false, false, true);

        callbacks.getUnchecked(0)->audioDeviceIOCallbackWithContext(
            inputChannelData, numInputChannels, outputChannelData,
            numOutputChannels, numSamples, context);

        auto *const *tempChans = tempBuffer.getArrayOfWritePointers();

        for (int i = callbacks.size(); --i > 0;)
        {
            callbacks.getUnchecked(i)->audioDeviceIOCallbackWithContext(
                inputChannelData, numInputChannels, tempChans,
                numOutputChannels, numSamples, context);

            for (int chan = 0; chan < numOutputChannels; ++chan)
            {
                if (const auto *src = tempChans[chan])
                {
                    if (auto *dst = outputChannelData[chan])
                    {
                        for (int j = 0; j < numSamples; ++j)
                        {
                            dst[j] += src[j];
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < numOutputChannels; ++i)
        {
            juce::zeromem(outputChannelData[i],
                          static_cast<size_t>(numSamples) * sizeof(float));
        }
    }

    if (testSound != nullptr)
    {
        const auto numSamps = juce::jmin(
            numSamples, testSound->getNumSamples() - testSoundPosition);
        auto *src = testSound->getReadPointer(0, testSoundPosition);

        for (int i = 0; i < numOutputChannels; ++i)
        {
            if (auto *dst = outputChannelData[i])
            {
                for (int j = 0; j < numSamps; ++j)
                {
                    dst[j] += src[j];
                }
            }
        }

        testSoundPosition += numSamps;

        if (testSoundPosition >= testSound->getNumSamples())
        {
            testSound.reset();
        }
    }

    outputLevelGetter->updateLevel(outputChannelData, numOutputChannels,
                                   numSamples);
}

void AudioDeviceManager::audioDeviceAboutToStartInt(
    juce::AudioIODevice *const device)
{
    loadMeasurer.reset(device->getCurrentSampleRate(),
                       device->getCurrentBufferSizeSamples());

    updateCurrentSetup();

    {
        const juce::ScopedLock sl(audioCallbackLock);

        for (int i = callbacks.size(); --i >= 0;)
        {
            callbacks.getUnchecked(i)->audioDeviceAboutToStart(device);
        }
    }

    sendChangeMessage();
}

void AudioDeviceManager::audioDeviceStoppedInt()
{
    sendChangeMessage();

    const juce::ScopedLock sl(audioCallbackLock);

    loadMeasurer.reset();

    for (int i = callbacks.size(); --i >= 0;)
    {
        callbacks.getUnchecked(i)->audioDeviceStopped();
    }
}

void AudioDeviceManager::audioDeviceErrorInt(const juce::String &message) const
{
    const juce::ScopedLock sl(audioCallbackLock);

    for (int i = callbacks.size(); --i >= 0;)
    {
        callbacks.getUnchecked(i)->audioDeviceError(message);
    }
}

double AudioDeviceManager::getCpuUsage() const
{
    return loadMeasurer.getLoadAsProportion();
}

//==============================================================================
void AudioDeviceManager::setMidiInputDeviceEnabled(
    const juce::String &identifier, const bool enabled)
{
    if (enabled != isMidiInputDeviceEnabled(identifier))
    {
        if (enabled)
        {
            if (auto midiIn = juce::MidiInput::openDevice(
                    identifier, callbackHandler.get()))
            {
                enabledMidiInputs.push_back(std::move(midiIn));
                enabledMidiInputs.back()->start();
            }
        }
        else
        {
            auto removePredicate =
                [identifier](const std::unique_ptr<juce::MidiInput> &in)
            {
                return in->getIdentifier() == identifier;
            };
            enabledMidiInputs.erase(
                std::remove_if(std::begin(enabledMidiInputs),
                               std::end(enabledMidiInputs), removePredicate),
                std::end(enabledMidiInputs));
        }

        updateXml();
        sendChangeMessage();
    }
}

bool AudioDeviceManager::isMidiInputDeviceEnabled(
    const juce::String &identifier) const
{
    for (auto &mi : enabledMidiInputs)
    {
        if (mi->getIdentifier() == identifier)
        {
            return true;
        }
    }

    return false;
}

void AudioDeviceManager::addMidiInputDeviceCallback(
    const juce::String &identifier, juce::MidiInputCallback *callbackToAdd)
{
    removeMidiInputDeviceCallback(identifier, callbackToAdd);

    if (identifier.isEmpty() || isMidiInputDeviceEnabled(identifier))
    {
        const juce::ScopedLock sl(midiCallbackLock);
        midiCallbacks.add({identifier, callbackToAdd});
    }
}

void AudioDeviceManager::removeMidiInputDeviceCallback(
    const juce::String &identifier,
    const juce::MidiInputCallback *callbackToRemove)
{
    for (int i = midiCallbacks.size(); --i >= 0;)
    {
        auto &mc = midiCallbacks.getReference(i);

        if (mc.callback == callbackToRemove &&
            mc.deviceIdentifier == identifier)
        {
            const juce::ScopedLock sl(midiCallbackLock);
            midiCallbacks.remove(i);
        }
    }
}

void AudioDeviceManager::handleIncomingMidiMessageInt(
    juce::MidiInput *source, const juce::MidiMessage &message)
{
    if (!message.isActiveSense())
    {
        const juce::ScopedLock sl(midiCallbackLock);

        for (auto &mc : midiCallbacks)
        {
            if (mc.deviceIdentifier.isEmpty() ||
                mc.deviceIdentifier == source->getIdentifier())
            {
                mc.callback->handleIncomingMidiMessage(source, message);
            }
        }
    }
}

//==============================================================================
void AudioDeviceManager::setDefaultMidiOutputDevice(
    const juce::String &identifier)
{
    if (defaultMidiOutputDeviceInfo.identifier != identifier)
    {
        std::unique_ptr<juce::MidiOutput> oldMidiPort;
        juce::Array<juce::AudioIODeviceCallback *> oldCallbacks;

        {
            const juce::ScopedLock sl(audioCallbackLock);
            oldCallbacks.swapWith(callbacks);
        }

        if (currentAudioDevice != nullptr)
        {
            for (int i = oldCallbacks.size(); --i >= 0;)
            {
                oldCallbacks.getUnchecked(i)->audioDeviceStopped();
            }
        }

        std::swap(oldMidiPort, defaultMidiOutput);

        if (identifier.isNotEmpty())
        {
            defaultMidiOutput = juce::MidiOutput::openDevice(identifier);
        }

        if (defaultMidiOutput != nullptr)
        {
            defaultMidiOutputDeviceInfo = defaultMidiOutput->getDeviceInfo();
        }
        else
        {
            defaultMidiOutputDeviceInfo = {};
        }

        if (currentAudioDevice != nullptr)
        {
            for (auto *c : oldCallbacks)
            {
                c->audioDeviceAboutToStart(currentAudioDevice.get());
            }
        }

        {
            const juce::ScopedLock sl(audioCallbackLock);
            oldCallbacks.swapWith(callbacks);
        }

        updateXml();
        sendSynchronousChangeMessage();
    }
}

//==============================================================================
AudioDeviceManager::LevelMeter::LevelMeter() noexcept : level() {}

void AudioDeviceManager::LevelMeter::updateLevel(
    const float *const *channelData, const int numChannels,
    const int numSamples) noexcept
{
    if (getReferenceCount() <= 1)
    {
        return;
    }

    auto localLevel = level.get();

    if (numChannels > 0)
    {
        for (int j = 0; j < numSamples; ++j)
        {
            float s = 0;

            for (int i = 0; i < numChannels; ++i)
            {
                s += std::abs(channelData[i][j]);
            }

            s /= static_cast<float>(numChannels);

            if (s > localLevel)
            {
                localLevel = s;
            }
            else if (localLevel > 0.001f)
            {
                constexpr float decayFactor = 0.99992f;
                localLevel *= decayFactor;
            }
            else
            {
                localLevel = 0;
            }
        }
    }
    else
    {
        localLevel = 0;
    }

    level = localLevel;
}

double AudioDeviceManager::LevelMeter::getCurrentLevel() const noexcept
{
    jassert(getReferenceCount() > 1);
    return level.get();
}

void AudioDeviceManager::playTestSound()
{
    {
        std::unique_ptr<juce::AudioBuffer<float>> oldSound;

        {
            const juce::ScopedLock sl(audioCallbackLock);
            std::swap(oldSound, testSound);
        }
    }

    testSoundPosition = 0;

    if (currentAudioDevice != nullptr)
    {
        const auto sampleRate = currentAudioDevice->getCurrentSampleRate();
        const auto soundLength = static_cast<int>(sampleRate);

        constexpr double frequency = 440.0;

        const auto phasePerSample =
            juce::MathConstants<double>::twoPi / (sampleRate / frequency);

        std::unique_ptr<juce::AudioBuffer<float>> newSound(
            new juce::AudioBuffer<float>(1, soundLength));

        for (int i = 0; i < soundLength; ++i)
        {
            constexpr float amplitude = 0.5f;
            newSound->setSample(
                0, i,
                amplitude * static_cast<float>(std::sin(i * phasePerSample)));
        }

        newSound->applyGainRamp(0, 0, soundLength / 10, 0.0f, 1.0f);
        newSound->applyGainRamp(0, soundLength - soundLength / 4,
                                soundLength / 4, 1.0f, 0.0f);

        {
            const juce::ScopedLock sl(audioCallbackLock);
            std::swap(testSound, newSound);
        }
    }
}

int AudioDeviceManager::getXRunCount() const noexcept
{
    const auto deviceXRuns =
        currentAudioDevice != nullptr ? currentAudioDevice->getXRunCount() : -1;
    return juce::jmax(0, deviceXRuns) + loadMeasurer.getXRunCount();
}
