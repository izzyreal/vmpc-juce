/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
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
