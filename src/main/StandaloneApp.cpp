#include "StandaloneApp.hpp"

#include "PropertiesFileOptions.hpp"

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

#if JUCE_IOS
void* juce_GetIOSCustomDelegateClass()  { return nullptr; }
#endif

using namespace vmpc_juce;

StandaloneApp::StandaloneApp()
{
    appProperties.setStorageParameters(vmpc_juce::PropertiesFileOptions());
}

StandaloneAppWindow* StandaloneApp::createWindow()
{
    StandalonePluginHolder::PluginInOuts channels[] = {{2, 10}};

    return new StandaloneAppWindow(getApplicationName(),
        LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId),
        appProperties.getUserSettings(),
        false,
        {},
        nullptr,
        juce::Array<StandalonePluginHolder::PluginInOuts>(channels, juce::numElementsInArray(channels)),
        true);
}

void StandaloneApp::initialise(const juce::String&)
{
    mainWindow.reset (createWindow());

   #if JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE
    juce::Desktop::getInstance().setKioskModeComponent (mainWindow.get(), false);
   #endif

    mainWindow->setVisible (true);
}

void StandaloneApp::shutdown()
{
#if JUCE_IOS
    if (mainWindow != nullptr)
        mainWindow->pluginHolder->savePluginState();
#endif
    mainWindow = nullptr;
    appProperties.saveIfNeeded();
}

void StandaloneApp::systemRequestedQuit()
{

#ifndef JUCE_IOS
    if (mainWindow != nullptr)
        mainWindow->pluginHolder->savePluginState();
#endif

    if (ModalComponentManager::getInstance()->cancelAllModalComponents())
        Timer::callAfterDelay(100, [&]() { requestQuit(); });
    else
        quit();
}

void StandaloneApp::requestQuit() const
{
    if (auto app = getInstance())
        app->systemRequestedQuit();
}


JUCEApplicationBase* juce_CreateApplication()
{
    return new StandaloneApp();
}
