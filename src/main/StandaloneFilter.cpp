#include "StandaloneFilter.h"
#include "ConfigurationHelpers.h"

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

#if JUCE_IOS
void* juce_GetIOSCustomDelegateClass()  { return nullptr; }
#endif

StandaloneFilterApp::StandaloneFilterApp()
{
    juce::PluginHostType::jucePlugInClientCurrentWrapperType =
        AudioProcessor::wrapperType_Standalone;
    appProperties.setStorageParameters(PropertiesFileOptions());
}

StandaloneFilterWindow* StandaloneFilterApp::createWindow()
{
    return new StandaloneFilterWindow(getApplicationName(),
        Helpers::getBackgroundColor(),
        appProperties.getUserSettings(),
        false,
        {},
        nullptr,
        Helpers::getChannelConfigurations(),
        Helpers::shouldAutoOpenMidiDevices());
}

void StandaloneFilterApp::initialise(const juce::String&)
{
    mainWindow.reset (createWindow());

   #if JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE
    juce::Desktop::getInstance().setKioskModeComponent (mainWindow.get(), false);
   #endif

    mainWindow->setVisible (true);
}

void StandaloneFilterApp::shutdown()
{
    mainWindow = nullptr;
    appProperties.saveIfNeeded();
}

void StandaloneFilterApp::systemRequestedQuit()
{
    if (mainWindow != nullptr)
        mainWindow->pluginHolder->savePluginState();

    if (ModalComponentManager::getInstance()->cancelAllModalComponents())
        Timer::callAfterDelay(100, [&]() { requestQuit(); });
    else
        quit();
}

void StandaloneFilterApp::requestQuit() const
{
    if (auto app = getInstance())
        app->systemRequestedQuit();
}


JUCEApplicationBase* juce_CreateApplication()
{
    return new StandaloneFilterApp();
}
