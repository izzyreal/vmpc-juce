#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector
{
    class LabelComponent : public juce::Component
    {
    public:
        virtual float getRequiredWidth() = 0;
        virtual float getRequiredHeight() = 0;

        virtual void setFontScale(const float fontScaleToUse)
        {
            fontScale = fontScaleToUse;
        }

    protected:
        float fontScale = 1.f;
    };
} // namespace vmpc_juce::gui::vector
