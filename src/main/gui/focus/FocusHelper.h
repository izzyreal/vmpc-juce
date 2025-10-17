#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <cstdint> 
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

namespace vmpc_juce::gui::focus {
    class FocusHelper : public juce::Component, juce::Timer {

        private:
            const std::function<void()> onFocusLost;

            bool focus = false;

            std::optional<bool> wasForeground;
            std::optional<bool> hadFocusedComponent;
            std::optional<bool> wasActiveWindow;
            std::optional<bool> peerWasValid;
            std::optional<bool> peerHandleWasValid;
            std::optional<bool> wasEditorFrontmost;
    
        public:
            FocusHelper(const std::function<void()> onFocusLostToUse) : onFocusLost(onFocusLostToUse)
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
            
            void timerCallback() override
            {
                const bool isForegroundProcess = juce::Process::isForegroundProcess();

                if (!wasForeground.has_value() || *wasForeground != isForegroundProcess)
                {
                    if (isForegroundProcess)
                    {
                        //MLOG("now is foreground");
                        //printf("now is foreground\n");
                    }
                    else
                    {
                        //MLOG("now is background");
                        //printf("now is background\n");
                    }

                    wasForeground = isForegroundProcess;
                }
                
                const auto* activeWindow = juce::TopLevelWindow::getActiveTopLevelWindow();
                const bool isActiveWindow = activeWindow != nullptr;
                
                if (!wasActiveWindow.has_value() || *wasActiveWindow != isActiveWindow)
                {
                    if (juce::Process::isForegroundProcess())
                    {
                        //MLOG("now is active window");
                        //printf("now is active window\n");
                    }
                    else
                    {
                        //MLOG("now is inactive window");
                        //printf("now is inactive window\n");
                    }
                    
                    wasActiveWindow = isActiveWindow;
                }
                
                const bool hasFocusedComponent = getCurrentlyFocusedComponent() != nullptr;
                
                if (!hadFocusedComponent.has_value() || *hadFocusedComponent != hasFocusedComponent)
                {
                    if (juce::Process::isForegroundProcess())
                    {
                        //MLOG("now has focused component");
                        //printf("now has focused component\n");
                    }
                    else
                    {
                        //MLOG("now has no focused component");
                        //printf("now has no focused component\n");
                    }

                    hadFocusedComponent = hasFocusedComponent;
                }
                
                const juce::ComponentPeer* peer = getPeer();
                const bool peerIsValid = peer != nullptr;
                
                if (!peerWasValid.has_value() || *peerWasValid != peerIsValid)
                {
                    if (peerIsValid)
                    {
                        //MLOG("peer is now valid");
                        //printf("peer is now valid\n");
                    }
                    else
                    {
                        //MLOG("peer is now invalid");
                        //printf("peer is now invalid\n");
                    }
                    
                    peerWasValid = peerIsValid;
                }

                bool peerHandleIsValid = false;
                bool isEditorFrontmost = false;
                
                if (peer != nullptr)
                {
                    void* peerNativeHandle = peer->getNativeHandle();
                    peerHandleIsValid = peerNativeHandle != nullptr;
                    
                    if (!peerHandleWasValid.has_value() || *peerHandleWasValid != peerHandleIsValid)
                    {
                        if (peerHandleIsValid)
                        {
                            //MLOG("peer native handle is now valid");
                            //printf("peer native handle is now valid\n");
                        }
                        else
                        {
                            //MLOG("peer native handle is now invalid");
                            //printf("peer native handle is now invalid\n");
                        }

                        peerHandleWasValid = peerHandleIsValid;
                    }

                    if (peerHandleIsValid)
                    {
                        isEditorFrontmost = isEditorKeyWindow(peerNativeHandle);
                        
                        if (!wasEditorFrontmost.has_value() || *wasEditorFrontmost != isEditorFrontmost)
                        {
                            if (isEditorFrontmost)
                            {
                                //MLOG("editor is now frontmost");
                                //printf("editor is now frontmost\n");
                            }
                            else
                            {
                                //MLOG("editor is now not frontmost");
                                //printf("editor is now not frontmost\n");
                            }
                            
                            wasEditorFrontmost = isEditorFrontmost;
                        }
                    }
                }
                
                bool newFocus;
                
                if (juce::PluginHostType::jucePlugInClientCurrentWrapperType == juce::AudioProcessor::wrapperType_AudioUnitv3)
                {
                    //MLOG("Setting focus for AUv3");
                    newFocus = isForegroundProcess &&
                               peerIsValid &&
                               peerHandleIsValid &&
                               isEditorFrontmost;
                }
                else if (juce::PluginHostType().isReaper())
                {
                    //MLOG("Setting focus for Reaper");
                    newFocus = isForegroundProcess &&
                               peerIsValid &&
                               peerHandleIsValid &&
                               isEditorFrontmost;
                }
                else
                {
                    //MLOG("Setting focus for not-Reaper");
                    newFocus = isForegroundProcess &&
                               isActiveWindow &&
                               hasFocusedComponent &&
                               peerIsValid &&
                               peerHandleIsValid &&
                               isEditorFrontmost;
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
                        if (!isForegroundProcess)   reason += "[background process] ";
                        if (!isActiveWindow)        reason += "[inactive window] ";
                        if (!hasFocusedComponent)   reason += "[no focused component] ";
                        if (!peerIsValid)           reason += "[invalid peer] ";
                        if (!peerHandleIsValid)     reason += "[invalid native handle] ";
                        if (!isEditorFrontmost)     reason += "[editor not frontmost] ";

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

