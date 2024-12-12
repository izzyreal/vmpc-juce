#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "VmpcProcessor.hpp"

#include "gui/bitmap/ContentComponent.hpp"
#include "gui/VmpcNoCornerResizerLookAndFeel.hpp"

namespace mpc { class Mpc; }

namespace vmpc_juce {
    class VmpcBitmapEditor
        : public juce::AudioProcessorEditor
    {

        public:
            explicit VmpcBitmapEditor(VmpcProcessor&);
            ~VmpcBitmapEditor() override;

            void resized() override;

        private:
            void showDisclaimer();

        private:
            VmpcNoCornerResizerLookAndFeel lookAndFeel;
            VmpcProcessor& vmpcProcessor;
            mpc::Mpc& mpc;

            juce::Viewport viewport;

            juce::TooltipWindow tooltipWindow { this, 300 };
            Component::SafePointer<juce::Component> vmpcSplashScreen;

            juce::Image bgImg;

            VmpcProcessor& getProcessor() const
            {
                return static_cast<VmpcProcessor&> (processor);
            }

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcBitmapEditor)
    };
} // namespace vmpc_juce
