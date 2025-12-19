#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "gui/WithSharedTimerCallback.hpp"

#include <optional>

#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS
extern "C" bool isEditorKeyWindow(void *componentPeerNativeHandle);
#elif TARGET_OS_OSX
extern "C" bool isEditorKeyWindow(void *componentPeerNativeHandle);
#endif
#elif _WIN32
extern "C" bool isEditorKeyWindow(void *componentPeerNativeHandle);
#elif __linux__
extern "C" bool isEditorKeyWindow(void *componentPeerNativeHandle);
#endif

#include "Logger.hpp"

namespace vmpc_juce::gui::focus
{
    class FocusHelper final : public juce::Component,
                              public WithSharedTimerCallback
    {
        const std::function<void()> onFocusLost;
        Component *auxComponent = nullptr;

        bool focus = false;

        std::optional<bool> wasForeground;
        std::optional<bool> hadFocusedComponent;
        std::optional<bool> wasActiveWindow;
        std::optional<bool> peerWasValid;
        std::optional<bool> peerHandleWasValid;
        std::optional<bool> wasEditorFrontmost;

    public:
        explicit FocusHelper(const std::function<void()> &onFocusLostToUse)
            : onFocusLost(onFocusLostToUse)
        {
            setIntervalMs(200);
        }

        ~FocusHelper() override
        {
            if (focus)
            {
                onFocusLost();
                focus = false;
            }
        }

        bool hasFocus() const
        {
            return focus;
        }

        void setAuxComponent(Component *comp)
        {
            auxComponent = comp;
        }

        void sharedTimerCallback() override
        {
            const bool isForegroundProcess =
                juce::Process::isForegroundProcess();

            const auto *activeWindow =
                juce::TopLevelWindow::getActiveTopLevelWindow();
            const bool isActiveWindow = activeWindow != nullptr;
            const bool hasFocusedComponent =
                getCurrentlyFocusedComponent() != nullptr;

            const juce::ComponentPeer *primaryPeer = getPeer();
            bool peerIsValid = primaryPeer != nullptr;
            bool peerHandleIsValid = false;
            bool isEditorFrontmost = false;

            if (primaryPeer)
            {
                void *primaryHandle = nullptr;
                primaryHandle = primaryPeer->getNativeHandle();
                peerHandleIsValid = primaryHandle != nullptr;
                if (peerHandleIsValid)
                {
                    isEditorFrontmost = isEditorKeyWindow(primaryHandle);
                }
            }

            if ((!peerIsValid || !peerHandleIsValid || !isEditorFrontmost) &&
                auxComponent)
            {
                if (const juce::ComponentPeer *auxPeer =
                        auxComponent->getPeer())
                {
                    void *auxHandle = auxPeer->getNativeHandle();
                    const bool auxHandleValid = auxHandle != nullptr;

                    if (auxHandleValid && isEditorKeyWindow(auxHandle))
                    {
                        peerIsValid = true;
                        peerHandleIsValid = true;
                        isEditorFrontmost = true;

                        MLOG("FocusHelper: using auxComponent as focus peer");
                    }
                }
            }

            bool newFocus = false;

            if (juce::PluginHostType::jucePlugInClientCurrentWrapperType ==
                juce::AudioProcessor::wrapperType_AudioUnitv3)
            {
                newFocus = isForegroundProcess && peerIsValid &&
                           peerHandleIsValid && isEditorFrontmost;
            }
            else if (juce::PluginHostType().isReaper())
            {
                newFocus = isForegroundProcess && peerIsValid &&
                           peerHandleIsValid && isEditorFrontmost;
            }
            else if (juce::PluginHostType().isAbletonLive())
            {
                newFocus = isForegroundProcess && peerIsValid &&
                           peerHandleIsValid && isEditorFrontmost;
            }
            else if (juce::PluginHostType().isBitwigStudio())
            {
                newFocus = isForegroundProcess && peerIsValid &&
                           peerHandleIsValid && isEditorFrontmost;
            }
            else if (juce::PluginHostType().isGarageBand())
            {
                newFocus = isForegroundProcess && peerIsValid &&
                           peerHandleIsValid && isEditorFrontmost;
            }
            else
            {
                newFocus = isForegroundProcess && isActiveWindow &&
                           hasFocusedComponent && peerIsValid &&
                           peerHandleIsValid && isEditorFrontmost;
            }

            if (focus && !newFocus)
            {
                onFocusLost();
            }

            if (newFocus != focus)
            {
                if (!newFocus)
                {
                    std::string reason = "NO FOCUS: ";
                    if (!isForegroundProcess)
                    {
                        reason += "[background process] ";
                    }
                    if (!isActiveWindow)
                    {
                        reason += "[inactive window] ";
                    }
                    if (!hasFocusedComponent)
                    {
                        reason += "[no focused component] ";
                    }
                    if (!peerIsValid)
                    {
                        reason += "[invalid peer] ";
                    }
                    if (!peerHandleIsValid)
                    {
                        reason += "[invalid native handle] ";
                    }
                    if (!isEditorFrontmost)
                    {
                        reason += "[editor not frontmost] ";
                    }
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
} // namespace vmpc_juce::gui::focus
