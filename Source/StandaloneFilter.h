#pragma once
#include "JuceHeader.h"
#include "StandaloneFilterWindow.h"
#include <juce_audio_plugin_client/utility/juce_CreatePluginFilter.h>
#include "PropertiesFileOptions.h"

extern JUCEApplicationBase* juce_CreateApplication();

using namespace juce;

class StandaloneFilterApp : public JUCEApplication
{
public:
    StandaloneFilterApp();

    const String getApplicationName() override { return JucePlugin_Name; }
    const String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void anotherInstanceStarted(const String&) override {}

    virtual StandaloneFilterWindow* createWindow();

    void initialise(const String&) override;
    void shutdown() override;
    void systemRequestedQuit() override;

private:
    ApplicationProperties appProperties;
    std::unique_ptr<StandaloneFilterWindow> mainWindow;
    void requestQuit() const;
};


