#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "standalone/StandaloneAppWindow.hpp"

extern juce::JUCEApplicationBase *juce_CreateApplication();

namespace vmpc_juce::standalone
{
    class StandaloneApp final : public juce::JUCEApplication,
                                private juce::Timer
    {
    public:
        StandaloneApp();

        const juce::String getApplicationName() override
        {
            return JucePlugin_Name;
        }
        const juce::String getApplicationVersion() override
        {
            return JucePlugin_VersionString;
        }
        bool moreThanOneInstanceAllowed() override
        {
            return false;
        }
        void anotherInstanceStarted(const juce::String &) override {}

        virtual StandaloneAppWindow *createWindow();

        void initialise(const juce::String &) override;
        void shutdown() override;
        void systemRequestedQuit() override;

    private:
        juce::ApplicationProperties appProperties;
        std::unique_ptr<StandaloneAppWindow> mainWindow;
        void requestQuit() const;
        void timerCallback() override;
        bool shutdownPending = false;
        double shutdownDeadlineMs = 0.0;
    };
} // namespace vmpc_juce::standalone
