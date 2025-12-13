#include "gui/vector/PadTimer.hpp"

#include "gui/vector/Pad.hpp"

using namespace vmpc_juce::gui::vector;

PadTimer::PadTimer(const std::vector<Pad *> &padsToUse) : pads(padsToUse)
{
    startTimer(5);
}

PadTimer::~PadTimer()
{
    stopTimer();
}

void PadTimer::timerCallback()
{
    for (const auto &p : pads)
    {
        p->padTimerCallback();
    }
}
