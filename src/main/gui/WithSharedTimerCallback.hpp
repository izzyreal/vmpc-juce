#pragma once

#include <utils/TimeUtils.hpp>

namespace vmpc_juce::gui
{
    class WithSharedTimerCallback
    {
    public:
        static constexpr int baseIntervalMs = 10;

        virtual ~WithSharedTimerCallback() = default;

        void timerCallback()
        {
            const auto now = mpc::utils::nowInMilliseconds();
            if (now - last >= intervalMs)
            {
                last = now;
                sharedTimerCallback();
            }
        }

    protected:
        void setIntervalMs(const int intervalMsToUse)
        {
            intervalMs = intervalMsToUse;
        }

        virtual void sharedTimerCallback() = 0;

    private:
        int intervalMs = 10;
        int64_t last = mpc::utils::nowInMilliseconds();
    };
} // namespace vmpc_juce::gui