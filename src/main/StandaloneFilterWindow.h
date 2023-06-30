#pragma once

#include "juce_audio_plugin_client/juce_audio_plugin_client.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include "juce_audio_utils/juce_audio_utils.h"

#include "VmpcProcessor.h"
#include "AudioMidiSettingsComponent.h"

namespace juce
{

class StandalonePluginHolder    : private AudioIODeviceCallback,
                                  private Timer,
                                  private Value::Listener,
                                  private juce::ComponentListener
{
public:
    struct PluginInOuts   { short numIns, numOuts; };

    StandalonePluginHolder (PropertySet* settingsToUse,
                            bool takeOwnershipOfSettings = true,
                            const String& preferredDefaultDeviceName = String(),
                            const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions = nullptr,
                            const Array<PluginInOuts>& channels = Array<PluginInOuts>(),
                           #if JUCE_ANDROID || JUCE_IOS
                            bool shouldAutoOpenMidiDevices = true
                           #else
                            bool shouldAutoOpenMidiDevices = false
                           #endif
                            )

        : settings (settingsToUse, takeOwnershipOfSettings),
          channelConfiguration (channels),
          autoOpenMidiDevices (shouldAutoOpenMidiDevices)
    {
        createPlugin();
        ((VmpcProcessor*) processor.get())->showAudioSettingsDialog = [this](){
            this->showAudioSettingsDialog();
        };

        auto inChannels = (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns
                                                           : processor->getMainBusNumInputChannels());

        if (preferredSetupOptions != nullptr)
            options.reset (new AudioDeviceManager::AudioDeviceSetup (*preferredSetupOptions));

        auto audioInputRequired = (inChannels > 0);

        if (audioInputRequired && RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
            && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
            RuntimePermissions::request (RuntimePermissions::recordAudio,
                                         [this, preferredDefaultDeviceName] (bool granted) { init (granted, preferredDefaultDeviceName); });
        else
            init (audioInputRequired, preferredDefaultDeviceName);
    }

    void init (bool enableAudioInput, const String& preferredDefaultDeviceName)
    {
        setupAudioDevices (enableAudioInput, preferredDefaultDeviceName, options.get());
        reloadPluginState();
        startPlaying();

       if (autoOpenMidiDevices)
           startTimer (500);
    }

    ~StandalonePluginHolder() override
    {
        stopTimer();

        deletePlugin();
        shutDownAudioDevices();
    }

    //==============================================================================
    virtual void createPlugin()
    {
        processor = createPluginFilterOfType (AudioProcessor::wrapperType_Standalone);
        processor->disableNonMainBuses();
        processor->setRateAndBufferSizeDetails (44100, 512);
    }

    virtual void deletePlugin()
    {
        stopPlaying();
        processor = nullptr;
    }

    int getNumInputChannels() const
    {
        if (processor == nullptr)
            return 0;

        return (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns
                                                : processor->getMainBusNumInputChannels());
    }

    int getNumOutputChannels() const
    {
        if (processor == nullptr)
            return 0;

        return (channelConfiguration.size() > 0 ? channelConfiguration[0].numOuts
                                                : processor->getMainBusNumOutputChannels());
    }

    static String getFilePatterns (const String& fileSuffix)
    {
        if (fileSuffix.isEmpty())
            return {};

        return (fileSuffix.startsWithChar ('.') ? "*" : "*.") + fileSuffix;
    }

    //==============================================================================
    void valueChanged (Value&) override            { /* not needed */ }

    //==============================================================================
    File getLastFile() const
    {
        File f;

        if (settings != nullptr)
            f = File (settings->getValue ("lastStateFile"));

        if (f == File())
            f = File::getSpecialLocation (File::userDocumentsDirectory);

        return f;
    }

    void startPlaying()
    {
        player.setProcessor (processor.get());

       #if JucePlugin_Enable_IAA && JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
        {
            processor->setPlayHead (device->getAudioPlayHead());
            device->setMidiMessageCollector (&player.getMidiMessageCollector());
        }
       #endif
    }

    void stopPlaying()
    {
        player.setProcessor (nullptr);
    }

    /** Shows an audio properties dialog box modally. */
    void showAudioSettingsDialog()
    {
        DialogWindow::LaunchOptions o;

        int maxNumInputs = 0, maxNumOutputs = 0;

        if (channelConfiguration.size() > 0)
        {
            auto& defaultConfig = channelConfiguration.getReference (0);

            maxNumInputs  = jmax (0, (int) defaultConfig.numIns);
            maxNumOutputs = jmax (0, (int) defaultConfig.numOuts);
        }

        if (auto* bus = processor->getBus (true, 0))
            maxNumInputs = jmax (maxNumInputs, bus->getDefaultLayout().size());

        if (auto* bus = processor->getBus (false, 0))
            maxNumOutputs = jmax (maxNumOutputs, bus->getDefaultLayout().size());

        auto content = std::make_unique<AudioMidiSettingsComponent> (deviceManager, maxNumInputs, maxNumOutputs);
        content->setSize (500, 550);
        content->setToRecommendedSize();
    
        o.content.setOwned (content.release());
        o.dialogTitle = "Audio/MIDI Settings";
        o.dialogBackgroundColour        = o.content->getLookAndFeel().findColour (ResizableWindow::backgroundColourId);
        o.escapeKeyTriggersCloseButton  = true;
        o.resizable                     = true;
        o.useBottomRightCornerResizer   = true;

#if JUCE_IOS
        o.useNativeTitleBar             = false;
#endif

        auto window = o.launchAsync();
        window->setComponentID("AudioMidiSettingsWindow");
        window->addComponentListener(this);
        if (!lastKnownAudioMidiSettingsWindowBounds.isEmpty())
        {
            window->setBounds(lastKnownAudioMidiSettingsWindowBounds);
        }
    }

    void componentMovedOrResized (Component &component, bool wasMoved, bool wasResized) override
    {
        if (!wasMoved && !wasResized) return;
        lastKnownAudioMidiSettingsWindowBounds = component.getBounds();
    }

    void saveAudioDeviceState()
    {
        if (settings != nullptr)
        {
            auto xml = deviceManager.createStateXml();

            settings->setValue ("audioSetup", xml.get());
        }
    }

    void reloadAudioDeviceState (bool enableAudioInput,
                                 const String& preferredDefaultDeviceName,
                                 const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)
    {
        std::unique_ptr<XmlElement> savedState;

        if (settings != nullptr)
        {
            savedState = settings->getXmlValue ("audioSetup");
        }

        auto inputChannels  = getNumInputChannels();
        auto outputChannels = getNumOutputChannels();

        if (inputChannels == 0 && outputChannels == 0 && processor->isMidiEffect())
        {
            // add a dummy output channel for MIDI effect plug-ins so they can receive audio callbacks
            outputChannels = 1;
        }

        deviceManager.initialise (enableAudioInput ? inputChannels : 0,
                                  outputChannels,
                                  savedState.get(),
                                  true,
                                  preferredDefaultDeviceName,
                                  preferredSetupOptions);
    }

    //==============================================================================
    void savePluginState()
    {
        if (settings != nullptr && processor != nullptr)
        {
            MemoryBlock data;
            processor->getStateInformation (data);

            settings->setValue ("filterState", data.toBase64Encoding());
        }
    }

    void reloadPluginState()
    {
        if (settings != nullptr)
        {
            MemoryBlock data;

            if (data.fromBase64Encoding (settings->getValue ("filterState")) && data.getSize() > 0)
                processor->setStateInformation (data.getData(), (int) data.getSize());
        }
    }

    //==============================================================================
    void switchToHostApplication()
    {
       #if JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
            device->switchApplication();
       #endif
    }

    bool isInterAppAudioConnected()
    {
       #if JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
            return device->isInterAppAudioConnected();
       #endif

        return false;
    }

    Image getIAAHostIcon ([[maybe_unused]] int size)
    {
       #if JUCE_IOS && JucePlugin_Enable_IAA
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
            return device->getIcon (size);
       #else
        ignoreUnused (size);
       #endif

        return {};
    }

    static StandalonePluginHolder* getInstance();

    //==============================================================================
    OptionalScopedPointer<PropertySet> settings;
    std::unique_ptr<AudioProcessor> processor;
    AudioDeviceManager deviceManager;
    AudioProcessorPlayer player;
    Array<PluginInOuts> channelConfiguration;
    juce::Rectangle<int> lastKnownAudioMidiSettingsWindowBounds;

    AudioBuffer<float> emptyBuffer;
    bool autoOpenMidiDevices;

    std::unique_ptr<AudioDeviceManager::AudioDeviceSetup> options;
    Array<MidiDeviceInfo> lastMidiDevices;

    std::unique_ptr<FileChooser> stateFileChooser;

private:
    /*  This class can be used to ensure that audio callbacks use buffers with a
        predictable maximum size.

        On some platforms (such as iOS 10), the expected buffer size reported in
        audioDeviceAboutToStart may be smaller than the blocks passed to
        audioDeviceIOCallbackWithContext. This can lead to out-of-bounds reads if the render
        callback depends on additional buffers which were initialised using the
        smaller size.

        As a workaround, this class will ensure that the render callback will
        only ever be called with a block with a length less than or equal to the
        expected block size.
    */
    class CallbackMaxSizeEnforcer  : public AudioIODeviceCallback
    {
    public:
        explicit CallbackMaxSizeEnforcer (AudioIODeviceCallback& callbackIn)
            : inner (callbackIn) {}

        void audioDeviceAboutToStart (AudioIODevice* device) override
        {
            maximumSize = device->getCurrentBufferSizeSamples();
            storedInputChannels .resize ((size_t) device->getActiveInputChannels() .countNumberOfSetBits());
            storedOutputChannels.resize ((size_t) device->getActiveOutputChannels().countNumberOfSetBits());

            inner.audioDeviceAboutToStart (device);
        }

        void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                               int numInputChannels,
                                               float* const* outputChannelData,
                                               int numOutputChannels,
                                               int numSamples,
                                               const AudioIODeviceCallbackContext& context) override
        {
            jassertquiet ((int) storedInputChannels.size()  == numInputChannels);
            jassertquiet ((int) storedOutputChannels.size() == numOutputChannels);

            int position = 0;

            while (position < numSamples)
            {
                const auto blockLength = jmin (maximumSize, numSamples - position);

                initChannelPointers (inputChannelData,  storedInputChannels,  position);
                initChannelPointers (outputChannelData, storedOutputChannels, position);

                inner.audioDeviceIOCallbackWithContext (storedInputChannels.data(),
                                                        (int) storedInputChannels.size(),
                                                        storedOutputChannels.data(),
                                                        (int) storedOutputChannels.size(),
                                                        blockLength,
                                                        context);

                position += blockLength;
            }
        }

        void audioDeviceStopped() override
        {
            inner.audioDeviceStopped();
        }

    private:
        struct GetChannelWithOffset
        {
            int offset;

            template <typename Ptr>
            auto operator() (Ptr ptr) const noexcept -> Ptr { return ptr + offset; }
        };

        template <typename Ptr, typename Vector>
        void initChannelPointers (Ptr&& source, Vector&& target, int offset)
        {
            std::transform (source, source + target.size(), target.begin(), GetChannelWithOffset { offset });
        }

        AudioIODeviceCallback& inner;
        int maximumSize = 0;
        std::vector<const float*> storedInputChannels;
        std::vector<float*> storedOutputChannels;
    };

    CallbackMaxSizeEnforcer maxSizeEnforcer { *this };

    //==============================================================================
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const AudioIODeviceCallbackContext& context) override
    {
        player.audioDeviceIOCallbackWithContext (inputChannelData,
                                                 numInputChannels,
                                                 outputChannelData,
                                                 numOutputChannels,
                                                 numSamples,
                                                 context);
    }

    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        emptyBuffer.setSize (device->getActiveInputChannels().countNumberOfSetBits(), device->getCurrentBufferSizeSamples());
        emptyBuffer.clear();

        player.audioDeviceAboutToStart (device);
        player.setMidiOutput (deviceManager.getDefaultMidiOutput());
    }

    void audioDeviceStopped() override
    {
        player.setMidiOutput (nullptr);
        player.audioDeviceStopped();
        emptyBuffer.setSize (0, 0);
    }

    //==============================================================================
    void setupAudioDevices (bool enableAudioInput,
                            const String& preferredDefaultDeviceName,
                            const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)
    {
        deviceManager.addAudioCallback (&maxSizeEnforcer);
        deviceManager.addMidiInputDeviceCallback ({}, &player);

        reloadAudioDeviceState (enableAudioInput, preferredDefaultDeviceName, preferredSetupOptions);
    }

    void shutDownAudioDevices()
    {
        saveAudioDeviceState();

        deviceManager.removeMidiInputDeviceCallback ({}, &player);
        deviceManager.removeAudioCallback (&maxSizeEnforcer);
    }

    void timerCallback() override
    {
        auto newMidiDevices = MidiInput::getAvailableDevices();

        if (newMidiDevices != lastMidiDevices)
        {
            for (auto& oldDevice : lastMidiDevices)
                if (! newMidiDevices.contains (oldDevice))
                    deviceManager.setMidiInputDeviceEnabled (oldDevice.identifier, false);

            for (auto& newDevice : newMidiDevices)
                if (! lastMidiDevices.contains (newDevice))
                    deviceManager.setMidiInputDeviceEnabled (newDevice.identifier, true);

            lastMidiDevices = newMidiDevices;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StandalonePluginHolder)
};

class StandaloneFilterWindow    : public DocumentWindow
{
public:
    typedef StandalonePluginHolder::PluginInOuts PluginInOuts;

    StandaloneFilterWindow (const String& title,
                            Colour backgroundColour,
                            PropertySet* settingsToUse,
                            bool takeOwnershipOfSettings,
                            const String& preferredDefaultDeviceName = String(),
                            const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions = nullptr,
                            const Array<PluginInOuts>& constrainToConfiguration = {},
                           #if JUCE_ANDROID || JUCE_IOS
                            bool autoOpenMidiDevices = true
                           #else
                            bool autoOpenMidiDevices = false
                           #endif
                            )
        : DocumentWindow (title, backgroundColour, DocumentWindow::minimiseButton | DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setConstrainer (&decoratorConstrainer);

       #if JUCE_IOS || JUCE_ANDROID
        setTitleBarHeight (0);
       #else
        setTitleBarButtonsRequired (DocumentWindow::minimiseButton | DocumentWindow::closeButton, false);
       #endif

        pluginHolder.reset (new StandalonePluginHolder (settingsToUse, takeOwnershipOfSettings,
                                                        preferredDefaultDeviceName, preferredSetupOptions,
                                                        constrainToConfiguration, autoOpenMidiDevices));

        class MidiOutputBackgroundThreadStarter : public juce::ChangeListener
        {
        private:
            MidiOutput* midiOutput = nullptr;
        public:
            void changeListenerCallback (ChangeBroadcaster* source) override
            {
                auto deviceManager = (AudioDeviceManager*)source;
                auto newMidiOutput = deviceManager->getDefaultMidiOutput();
                if (midiOutput != newMidiOutput)
                {
                    midiOutput = newMidiOutput;

                    if (midiOutput != nullptr)
                    {
                        midiOutput->startBackgroundThread();
                    }
                }
            }
        };

        class MidiPanicUponAudioDeviceChangeListener : public juce::ChangeListener
        {
        private:
            mpc::Mpc& mpc;
        public:
            MidiPanicUponAudioDeviceChangeListener(mpc::Mpc& mpcToUse) : mpc(mpcToUse) {}
            void changeListenerCallback (ChangeBroadcaster* source) override
            {
                mpc.panic();
            }
        };

        pluginHolder->deviceManager.addChangeListener(new MidiOutputBackgroundThreadStarter());
        pluginHolder->deviceManager.sendSynchronousChangeMessage();

        auto& mpc = dynamic_cast<VmpcProcessor*>(pluginHolder->processor.get())->mpc;
        pluginHolder->deviceManager.addChangeListener(new MidiPanicUponAudioDeviceChangeListener(mpc));

        #if JUCE_IOS || JUCE_ANDROID
        setFullScreen (true);
        updateContent();
        #else
        updateContent();

        const auto windowScreenBounds = [this]() -> Rectangle<int>
        {
            const auto width = getWidth();
            const auto height = getHeight();

            const auto& displays = Desktop::getInstance().getDisplays();

            if (auto* props = pluginHolder->settings.get())
            {
                constexpr int defaultValue = -100;

                const auto x = props->getIntValue ("windowX", defaultValue);
                const auto y = props->getIntValue ("windowY", defaultValue);

                if (x != defaultValue && y != defaultValue)
                {
                    const auto screenLimits = displays.getDisplayForRect ({ x, y, width, height })->userArea;

                    return { jlimit (screenLimits.getX(), jmax (screenLimits.getX(), screenLimits.getRight()  - width),  x),
                             jlimit (screenLimits.getY(), jmax (screenLimits.getY(), screenLimits.getBottom() - height), y),
                             width, height };
                }
            }

            const auto displayArea = displays.getPrimaryDisplay()->userArea;

            return { displayArea.getCentreX() - width / 2,
                     displayArea.getCentreY() - height / 2,
                     width, height };
        }();

        setBoundsConstrained (windowScreenBounds);

        if (auto* processor = getAudioProcessor())
            if (auto* editor = processor->getActiveEditor())
                setResizable (editor->isResizable(), false);
       #endif
    }

    ~StandaloneFilterWindow() override
    {
       #if (! JUCE_IOS) && (! JUCE_ANDROID)
        if (auto* props = pluginHolder->settings.get())
        {
            props->setValue ("windowX", getX());
            props->setValue ("windowY", getY());
        }
       #endif

        pluginHolder->stopPlaying();
        clearContentComponent();
        pluginHolder = nullptr;
    }

    //==============================================================================
    AudioProcessor* getAudioProcessor() const noexcept      { return pluginHolder->processor.get(); }
    AudioDeviceManager& getDeviceManager() const noexcept   { return pluginHolder->deviceManager; }

    /** Deletes and re-creates the plugin, resetting it to its default state. */
    void resetToDefaultState()
    {
        pluginHolder->stopPlaying();
        clearContentComponent();
        pluginHolder->deletePlugin();

        if (auto* props = pluginHolder->settings.get())
            props->removeValue ("filterState");

        pluginHolder->createPlugin();
        updateContent();
        pluginHolder->startPlaying();
    }

    //==============================================================================
    void closeButtonPressed() override
    {
        pluginHolder->savePluginState();

        JUCEApplicationBase::quit();
    }

    virtual StandalonePluginHolder* getPluginHolder()    { return pluginHolder.get(); }

    std::unique_ptr<StandalonePluginHolder> pluginHolder;

private:
    void updateContent()
    {
        auto* content = new MainContentComponent (*this);
        decoratorConstrainer.setMainContentComponent (content);

       #if JUCE_IOS || JUCE_ANDROID
        constexpr auto resizeAutomatically = false;
       #else
        constexpr auto resizeAutomatically = true;
       #endif

        setContentOwned (content, resizeAutomatically);
    }

    //==============================================================================
    class MainContentComponent  : public Component,
                                  private Value::Listener,
                                  private ComponentListener
    {
    public:
        MainContentComponent (StandaloneFilterWindow& filterWindow)
            : owner (filterWindow),
              editor (owner.getAudioProcessor()->hasEditor() ? owner.getAudioProcessor()->createEditorIfNeeded()
                                                             : new GenericAudioProcessorEditor (*owner.getAudioProcessor()))
        {
            if (editor != nullptr)
            {
                editor->addComponentListener (this);
                componentMovedOrResized (*editor, false, true);

                addAndMakeVisible (editor.get());
            }
        }

        ~MainContentComponent() override
        {
            if (editor != nullptr)
            {
                editor->removeComponentListener (this);
                owner.pluginHolder->processor->editorBeingDeleted (editor.get());
                editor = nullptr;
            }
        }

        void resized() override
        {
            auto r = getLocalBounds();

            if (editor != nullptr)
            {
                const auto newPos = r.getTopLeft().toFloat().transformedBy (editor->getTransform().inverted());

                if (preventResizingEditor)
                    editor->setTopLeftPosition (newPos.roundToInt());
                else
                    editor->setBoundsConstrained (editor->getLocalArea (this, r.toFloat()).withPosition (newPos).toNearestInt());
            }
        }

        ComponentBoundsConstrainer* getEditorConstrainer() const
        {
            if (auto* e = editor.get())
                return e->getConstrainer();

            return nullptr;
        }

        BorderSize<int> computeBorder() const
        {
            const auto nativeFrame = [&]() -> BorderSize<int>
            {
                if (auto* peer = owner.getPeer())
                    if (const auto frameSize = peer->getFrameSizeIfPresent())
                        return *frameSize;

                return {};
            }();

            return nativeFrame.addedTo (owner.getContentComponentBorder());
        }

    private:
        void valueChanged (Value&) override     { /* not needed */ }

        //==============================================================================
        void componentMovedOrResized (Component&, bool, bool) override
        {
            const ScopedValueSetter<bool> scope (preventResizingEditor, true);

            if (editor != nullptr)
            {
                auto rect = getSizeToContainEditor();

                setSize (rect.getWidth(),
                         rect.getHeight());
            }
        }

        Rectangle<int> getSizeToContainEditor() const
        {
            if (editor != nullptr)
                return getLocalArea (editor.get(), editor->getLocalBounds());

            return {};
        }

        //==============================================================================
        StandaloneFilterWindow& owner;
        std::unique_ptr<AudioProcessorEditor> editor;
        bool shouldShowNotification = false;
        bool preventResizingEditor = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
    };

    /*  This custom constrainer checks with the AudioProcessorEditor (which might itself be
        constrained) to ensure that any size we choose for the standalone window will be suitable
        for the editor too.

        Without this constrainer, attempting to resize the standalone window may set bounds on the
        peer that are unsupported by the inner editor. In this scenario, the peer will be set to a
        'bad' size, then the inner editor will be resized. The editor will check the new bounds with
        its own constrainer, and may set itself to a more suitable size. After that, the resizable
        window will see that its content component has changed size, and set the bounds of the peer
        accordingly. The end result is that the peer is resized twice in a row to different sizes,
        which can appear glitchy/flickery to the user.
    */
    struct DecoratorConstrainer : public ComponentBoundsConstrainer
    {
        void checkBounds (Rectangle<int>& bounds,
                          const Rectangle<int>& previousBounds,
                          const Rectangle<int>& limits,
                          bool isStretchingTop,
                          bool isStretchingLeft,
                          bool isStretchingBottom,
                          bool isStretchingRight) override
        {
            auto* decorated = contentComponent != nullptr ? contentComponent->getEditorConstrainer()
                                                          : nullptr;

            if (decorated != nullptr)
            {
                const auto border = contentComponent->computeBorder();
                const auto requestedBounds = bounds;

                border.subtractFrom (bounds);
                decorated->checkBounds (bounds,
                                        border.subtractedFrom (previousBounds),
                                        limits,
                                        isStretchingTop,
                                        isStretchingLeft,
                                        isStretchingBottom,
                                        isStretchingRight);
                border.addTo (bounds);
                bounds = bounds.withPosition (requestedBounds.getPosition());

                if (isStretchingTop && ! isStretchingBottom)
                    bounds = bounds.withBottomY (previousBounds.getBottom());

                if (! isStretchingTop && isStretchingBottom)
                    bounds = bounds.withY (previousBounds.getY());

                if (isStretchingLeft && ! isStretchingRight)
                    bounds = bounds.withRightX (previousBounds.getRight());

                if (! isStretchingLeft && isStretchingRight)
                    bounds = bounds.withX (previousBounds.getX());
            }
            else
            {
                ComponentBoundsConstrainer::checkBounds (bounds,
                                                         previousBounds,
                                                         limits,
                                                         isStretchingTop,
                                                         isStretchingLeft,
                                                         isStretchingBottom,
                                                         isStretchingRight);
            }
        }

        void setMainContentComponent (MainContentComponent* in) { contentComponent = in; }

    private:
        MainContentComponent* contentComponent = nullptr;
    };

    //==============================================================================
    DecoratorConstrainer decoratorConstrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StandaloneFilterWindow)
};

inline StandalonePluginHolder* StandalonePluginHolder::getInstance()
{
   #if JucePlugin_Enable_IAA || JucePlugin_Build_Standalone
    if (PluginHostType::getPluginLoadedAs() == AudioProcessor::wrapperType_Standalone)
    {
        auto& desktop = Desktop::getInstance();
        const int numTopLevelWindows = desktop.getNumComponents();

        for (int i = 0; i < numTopLevelWindows; ++i)
            if (auto window = dynamic_cast<StandaloneFilterWindow*> (desktop.getComponent (i)))
                return window->getPluginHolder();
    }
   #endif

    return nullptr;
}

} // namespace juce
