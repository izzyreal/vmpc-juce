#pragma once

#include "SvgComponent.hpp"
#include "PadGlowState.hpp"

#include <IntTypes.hpp>

#include <memory>

namespace mpc
{
    class Mpc;
}

namespace mpc::hardware
{
    class Pad;
}

namespace vmpc_juce::gui::vector
{
    class Pad final : public SvgComponent, public juce::FileDragAndDropTarget
    {
    private:
        PadGlowState glowState;

        mpc::Mpc &mpc;
        std::shared_ptr<mpc::hardware::Pad> mpcPad;
        SvgComponent *glowSvg = nullptr;

        int getVelo(int veloY) const;
        void loadFile(const juce::String &path, bool shouldBeConverted);
        std::function<float()> getScale;
        bool mutatedSinceLastPaint = false;
        bool pressedPadSvgVisible = false;
        int fadeFrameCounter = 0;
        const int fadeRepaintInterval = 5;

        void updatePressedPadSvg();

    public:
        void registerPhysicalPress(mpc::Velocity);
        void registerPhysicalAftertouch(mpc::Pressure);
        void registerPhysicalRelease();
        void registerProgramPress(mpc::ProgramPadIndex, mpc::Velocity);
        void registerProgramAftertouch(mpc::ProgramPadIndex, mpc::Pressure);
        void registerProgramRelease(mpc::ProgramPadIndex);
        void setActiveBank(mpc::controller::Bank);
        void resized() override;
        void mouseDrag(const juce::MouseEvent &) override;
        void padTimerCallback();
        bool isInterestedInFileDrag(const juce::StringArray &files) override;
        void filesDropped(const juce::StringArray &files, int x,
                          int y) override;
        void paint(juce::Graphics &g) override;

        Pad(juce::Component *commonParentWithShadowToUse, float shadowSizeToUse,
            const std::function<float()> &getScaleToUse, mpc::Mpc &,
            std::shared_ptr<mpc::hardware::Pad>);

        ~Pad() override;
    };

} // namespace vmpc_juce::gui::vector
