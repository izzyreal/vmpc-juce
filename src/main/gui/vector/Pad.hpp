#pragma once

#include "SvgComponent.hpp"

#include <memory>
#include <optional>

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

    class Pad : public SvgComponent, public juce::FileDragAndDropTarget
    {
    private:
        struct Press
        {
            int padIndexWithBank;
            float alpha;
            int veloOrPressure;
            enum class Phase { Immediate, Sustained, Releasing } phase = Phase::Immediate;
            std::chrono::steady_clock::time_point pressTime;
            bool wasPaintedWithInitialAlpha = false;
            float getAlphaWithVeloApplied() const
            {
                return alpha * (static_cast<float>(std::clamp(veloOrPressure, 50, 127)) / 127.f);
            }
        };

        std::optional<Press> primaryPress;
        std::optional<Press> secondaryPress;
        std::optional<Press> tertiaryPress;

        int lastBank = -1;

        mpc::Mpc &mpc;
        std::shared_ptr<mpc::hardware::Pad> mpcPad;
        SvgComponent *glowSvg = nullptr;
        std::optional<int> pressedBank = std::nullopt;

        int getVelo(int veloY);
        void loadFile(const juce::String path, bool shouldBeConverted);
        std::function<float()> getScale;
        bool mutatedSinceLastPaint = false;
        int fadeFrameCounter = 0;
        const int fadeRepaintInterval = 3;

    public:
        void resized() override;
        void mouseDrag(const juce::MouseEvent &event) override;
        void sharedTimerCallback();
        bool isInterestedInFileDrag(const juce::StringArray &files) override;
        void filesDropped(const juce::StringArray &files, int x, int y) override;
        void paint(juce::Graphics &g) override;

        Pad(juce::Component *commonParentWithShadowToUse,
            float shadowSizeToUse,
            const std::function<float()> &getScaleToUse,
            mpc::Mpc &,
            std::shared_ptr<mpc::hardware::Pad>);

        ~Pad() override;
    };

} // namespace vmpc_juce::gui::vector

