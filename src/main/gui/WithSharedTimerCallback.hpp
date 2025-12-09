#pragma once

namespace vmpc_juce::gui
{
    class WithSharedTimerCallback
    {
    public:
        virtual ~WithSharedTimerCallback() = default;
        virtual void sharedTimerCallback() = 0;
    };
}