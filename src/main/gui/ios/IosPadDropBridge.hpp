#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#if JUCE_IOS
namespace vmpc_juce::gui::ios
{
    class IosPadDropBridge
    {
    public:
        explicit IosPadDropBridge(juce::Component &editor);
        ~IosPadDropBridge();

        void refreshPeer();

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };
} // namespace vmpc_juce::gui::ios
#endif
