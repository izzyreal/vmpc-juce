#pragma once

#include <juce_events/juce_events.h>
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace vmpc_juce::standalone
{
    class AudioDeviceManager final : public juce::ChangeBroadcaster
    {
    public:
        AudioDeviceManager();

        ~AudioDeviceManager() override;
        struct AudioDeviceSetup
        {
            juce::String outputDeviceName;

            juce::String inputDeviceName;

            double sampleRate = 0;

            int bufferSize = 0;

            juce::BigInteger inputChannels;

            bool useDefaultInputChannels = true;

            juce::BigInteger outputChannels;

            bool useDefaultOutputChannels = true;

            bool operator==(const AudioDeviceSetup &) const;
            bool operator!=(const AudioDeviceSetup &) const;
        };

        juce::String initialise(
            int numInputChannelsNeeded, int numOutputChannelsNeeded,
            const juce::XmlElement *savedState,
            bool selectDefaultDeviceOnFailure,
            const juce::String &preferredDefaultDeviceName = juce::String(),
            const AudioDeviceSetup *preferredSetupOptions = nullptr);

        juce::String initialiseWithDefaultDevices(int numInputChannelsNeeded,
                                                  int numOutputChannelsNeeded);

        std::unique_ptr<juce::XmlElement> createStateXml() const;

        AudioDeviceSetup getAudioDeviceSetup() const;

        void getAudioDeviceSetup(AudioDeviceSetup &result) const;

        juce::String setAudioDeviceSetup(const AudioDeviceSetup &newSetup,
                                         bool treatAsChosenDevice);

        juce::AudioIODevice *getCurrentAudioDevice() const noexcept
        {
            return currentAudioDevice.get();
        }

        juce::String getCurrentAudioDeviceType() const
        {
            return currentDeviceType;
        }

        juce::AudioIODeviceType *getCurrentDeviceTypeObject() const;

        void setCurrentAudioDeviceType(const juce::String &type,
                                       bool treatAsChosenDevice);

        juce::AudioWorkgroup getDeviceAudioWorkgroup() const;

        void closeAudioDevice();

        void restartLastAudioDevice();

        void addAudioCallback(juce::AudioIODeviceCallback *newCallback);

        void removeAudioCallback(juce::AudioIODeviceCallback *callback);

        double getCpuUsage() const;

        void setMidiInputDeviceEnabled(const juce::String &deviceIdentifier,
                                       bool enabled);

        bool
        isMidiInputDeviceEnabled(const juce::String &deviceIdentifier) const;

        void addMidiInputDeviceCallback(const juce::String &deviceIdentifier,
                                        juce::MidiInputCallback *callback);

        void
        removeMidiInputDeviceCallback(const juce::String &deviceIdentifier,
                                      const juce::MidiInputCallback *callback);

        void setDefaultMidiOutputDevice(const juce::String &deviceIdentifier);

        const juce::String &getDefaultMidiOutputIdentifier() const noexcept
        {
            return defaultMidiOutputDeviceInfo.identifier;
        }

        juce::MidiOutput *getDefaultMidiOutput() const noexcept
        {
            return defaultMidiOutput.get();
        }

        const juce::OwnedArray<juce::AudioIODeviceType> &
        getAvailableDeviceTypes();

        void createAudioDeviceTypes(
            juce::OwnedArray<juce::AudioIODeviceType> &types) const;

        void addAudioDeviceType(
            std::unique_ptr<juce::AudioIODeviceType> newDeviceType);

        void removeAudioDeviceType(
            const juce::AudioIODeviceType *deviceTypeToRemove);

        void playTestSound();

        struct LevelMeter final : juce::ReferenceCountedObject
        {
            LevelMeter() noexcept;
            double getCurrentLevel() const noexcept;

            using Ptr = juce::ReferenceCountedObjectPtr<LevelMeter>;

            void resetToZeroLevel() noexcept;

        private:
            friend class AudioDeviceManager;

            juce::Atomic<float> level{0};
            void updateLevel(const float *const *, int numChannels,
                             int numSamples) noexcept;
        };

        LevelMeter::Ptr getInputLevelGetter() noexcept
        {
            return inputLevelGetter;
        }

        LevelMeter::Ptr getOutputLevelGetter() noexcept
        {
            return outputLevelGetter;
        }

        juce::CriticalSection &getAudioCallbackLock() noexcept
        {
            return audioCallbackLock;
        }

        juce::CriticalSection &getMidiCallbackLock() noexcept
        {
            return midiCallbackLock;
        }

        int getXRunCount() const noexcept;

    private:
        juce::OwnedArray<juce::AudioIODeviceType> availableDeviceTypes;
        juce::OwnedArray<AudioDeviceSetup> lastDeviceTypeConfigs;

        AudioDeviceSetup currentSetup;
        std::unique_ptr<juce::AudioIODevice> currentAudioDevice;
        juce::Array<juce::AudioIODeviceCallback *> callbacks;
        int numInputChansNeeded = 0, numOutputChansNeeded = 2;
        juce::String preferredDeviceName, currentDeviceType;
        std::unique_ptr<juce::XmlElement> lastExplicitSettings;
        mutable bool listNeedsScanning = true;
        juce::AudioBuffer<float> tempBuffer;
        juce::MidiDeviceListConnection midiDeviceListConnection =
            juce::MidiDeviceListConnection::make(
                [this]
                {
                    midiDeviceListChanged();
                });

        struct MidiCallbackInfo
        {
            juce::String deviceIdentifier;
            juce::MidiInputCallback *callback;
        };

        juce::Array<juce::MidiDeviceInfo> midiDeviceInfosFromXml;
        std::vector<std::unique_ptr<juce::MidiInput>> enabledMidiInputs;
        juce::Array<MidiCallbackInfo> midiCallbacks;

        juce::MidiDeviceInfo defaultMidiOutputDeviceInfo;
        std::unique_ptr<juce::MidiOutput> defaultMidiOutput;
        juce::CriticalSection audioCallbackLock, midiCallbackLock;

        std::unique_ptr<juce::AudioBuffer<float>> testSound;
        int testSoundPosition = 0;

        juce::AudioProcessLoadMeasurer loadMeasurer;

        LevelMeter::Ptr inputLevelGetter{new LevelMeter()},
            outputLevelGetter{new LevelMeter()};

        class CallbackHandler;
        std::unique_ptr<CallbackHandler> callbackHandler;

        void audioDeviceIOCallbackInt(
            const float *const *inputChannelData, int totalNumInputChannels,
            float *const *outputChannelData, int totalNumOutputChannels,
            int numSamples, const juce::AudioIODeviceCallbackContext &context);
        void audioDeviceAboutToStartInt(juce::AudioIODevice *);
        void audioDeviceStoppedInt();
        void audioDeviceErrorInt(const juce::String &) const;
        void handleIncomingMidiMessageInt(juce::MidiInput *,
                                          const juce::MidiMessage &);
        void audioDeviceListChanged();
        void midiDeviceListChanged();

        void stopDevice();

        void updateXml();

        void updateCurrentSetup();
        void createDeviceTypesIfNeeded();
        void scanDevicesIfNeeded();
        void deleteCurrentDevice();
        double chooseBestSampleRate(double preferred) const;
        int chooseBestBufferSize(int preferred) const;
        void insertDefaultDeviceNames(AudioDeviceSetup &) const;
        juce::String
        initialiseDefault(const juce::String &preferredDefaultDeviceName,
                          const AudioDeviceSetup *);
        juce::String
        initialiseFromXML(const juce::XmlElement &,
                          bool selectDefaultDeviceOnFailure,
                          const juce::String &preferredDefaultDeviceName,
                          const AudioDeviceSetup *);
        void
        openLastRequestedMidiDevices(const juce::Array<juce::MidiDeviceInfo> &,
                                     const juce::MidiDeviceInfo &);

        juce::AudioIODeviceType *findType(const juce::String &inputName,
                                          const juce::String &outputName);
        juce::AudioIODeviceType *findType(const juce::String &typeName);
        void pickCurrentDeviceTypeWithDevices();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeviceManager)
    };

} // namespace vmpc_juce::standalone
