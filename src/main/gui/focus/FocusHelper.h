#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

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

namespace vmpc_juce::gui::focus {
    class FocusHelper : juce::Component, juce::Timer {

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
                startTimer(100);
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

                const bool newFocus =
                    isForegroundProcess &&
                    isActiveWindow &&
                    hasFocusedComponent &&
                    peerIsValid &&
                    peerHandleIsValid &&
                    isEditorFrontmost;

                if (focus && !newFocus)
                {
                    onFocusLost();
                }

                focus = newFocus;
            }
    };
}

