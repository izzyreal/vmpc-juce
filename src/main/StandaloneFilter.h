#pragma once
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include "StandaloneFilterWindow.h"
#include "PropertiesFileOptions.h"
//
//#include <juce_audio_basics/juce_audio_basics.h>
//#include <juce_audio_devices/juce_audio_devices.h>
//#include <juce_audio_formats/juce_audio_formats.h>
//#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
//#include <juce_audio_processors/juce_audio_processors.h>
//#include <juce_audio_utils/juce_audio_utils.h>
//#include <juce_core/juce_core.h>
//#include <juce_data_structures/juce_data_structures.h>
//#include <juce_events/juce_events.h>
//#include <juce_graphics/juce_graphics.h>
//#include <juce_gui_basics/juce_gui_basics.h>
//#include <juce_gui_extra/juce_gui_extra.h>

extern JUCEApplicationBase* juce_CreateApplication();

using namespace juce;

class StandaloneFilterApp : public JUCEApplication
{
public:
    StandaloneFilterApp();

    const String getApplicationName() override { return JucePlugin_Name; }
    const String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override { return false; }
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


