#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <optional>

#if __APPLE__
  #include <TargetConditionals.h>
  #if TARGET_OS_IOS
    extern "C" bool isEditorKeyWindow(void* componentPeerNativeHandle);
  #elif TARGET_OS_OSX
    extern "C" bool isEditorKeyWindow(void* componentPeerNativeHandle);
  #endif
#elif _WIN32
  extern "C" bool isEditorKeyWindow(void* componentPeerNativeHandle);
#elif __linux__
  extern "C" bool isEditorKeyWindow(void* componentPeerNativeHandle);
#endif

#include "Logger.hpp"

namespace vmpc_juce::gui::focus
{
    class FocusHelper : public juce::Component, juce::Timer
    {
    private:
        const std::function<void()> onFocusLost;
        juce::Component* auxComponent = nullptr;

        bool focus = false;

        std::optional<bool> wasForeground;
        std::optional<bool> hadFocusedComponent;
        std::optional<bool> wasActiveWindow;
        std::optional<bool> peerWasValid;
        std::optional<bool> peerHandleWasValid;
        std::optional<bool> wasEditorFrontmost;

    public:
        FocusHelper(const std::function<void()> onFocusLostToUse)
            : onFocusLost(onFocusLostToUse)
        {
            startTimer(50);
        }

        ~FocusHelper() override
        {
            stopTimer();

            if (focus)
            {
                onFocusLost();
                focus = false;
            }
        }

        bool hasFocus() const { return focus; }

        void setAuxComponent(juce::Component* comp) { auxComponent = comp; }

        void timerCallback() override
        {
            const bool isForegroundProcess = juce::Process::isForegroundProcess();

            const auto* activeWindow = juce::TopLevelWindow::getActiveTopLevelWindow();
            const bool isActiveWindow = activeWindow != nullptr;
            const bool hasFocusedComponent = getCurrentlyFocusedComponent() != nullptr;

            const juce::ComponentPeer* primaryPeer = getPeer();
            bool peerIsValid = primaryPeer != nullptr;
            bool peerHandleIsValid = false;
            bool isEditorFrontmost = false;

            void* primaryHandle = nullptr;

            if (primaryPeer)
            {
                primaryHandle = primaryPeer->getNativeHandle();
                peerHandleIsValid = primaryHandle != nullptr;
                if (peerHandleIsValid)
                    isEditorFrontmost = isEditorKeyWindow(primaryHandle);
            }

            if ((!peerIsValid || !peerHandleIsValid || !isEditorFrontmost) && auxComponent)
            {
                const juce::ComponentPeer* auxPeer = auxComponent->getPeer();
                const bool auxPeerValid = auxPeer != nullptr;

                if (auxPeerValid)
                {
                    void* auxHandle = auxPeer->getNativeHandle();
                    const bool auxHandleValid = auxHandle != nullptr;

                    if (auxHandleValid && isEditorKeyWindow(auxHandle))
                    {
                        // substitute aux peer completely
                        peerIsValid = true;
                        peerHandleIsValid = true;
                        isEditorFrontmost = true;

                        // optional: mark so logs can show fallback was used
                        MLOG("FocusHelper: using auxComponent as focus peer");
                    }
                }
            }

            bool newFocus = false;

            if (juce::PluginHostType::jucePlugInClientCurrentWrapperType ==
                juce::AudioProcessor::wrapperType_AudioUnitv3)
            {
                // AUv3 case
                newFocus = isForegroundProcess &&
                           peerIsValid &&
                           peerHandleIsValid &&
                           isEditorFrontmost;
            }
            else if (juce::PluginHostType().isReaper())
            {
                // Reaper-specific rule
                newFocus = isForegroundProcess &&
                           peerIsValid &&
                           peerHandleIsValid &&
                           isEditorFrontmost;
            }
            else
            {
                // Generic / other hosts
                newFocus = isForegroundProcess &&
                           isActiveWindow &&
                           hasFocusedComponent &&
                           peerIsValid &&
                           peerHandleIsValid &&
                           isEditorFrontmost;
            }

            // --- handle transitions ---
            if (focus && !newFocus)
            {
                onFocusLost();
            }

            if (newFocus != focus)
            {
                if (!newFocus)
                {
                    std::string reason = "NO FOCUS: ";
                    if (!isForegroundProcess) reason += "[background process] ";
                    if (!isActiveWindow) reason += "[inactive window] ";
                    if (!hasFocusedComponent) reason += "[no focused component] ";
                    if (!peerIsValid) reason += "[invalid peer] ";
                    if (!peerHandleIsValid) reason += "[invalid native handle] ";
                    if (!isEditorFrontmost) reason += "[editor not frontmost] ";
                    MLOG(reason);
                }
                else
                {
                    MLOG("we have focus");
                }
            }

            focus = newFocus;
        }
    };
}

