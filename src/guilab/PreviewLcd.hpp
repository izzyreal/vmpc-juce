#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::guilab
{
    class PreviewLcd final : public juce::Component
    {
    public:
        PreviewLcd();
        void paint(juce::Graphics &g) override;

        float magicMultiplier = 1.f;

    private:
        juce::Image pixels;
    };
} // namespace vmpc_juce::guilab
