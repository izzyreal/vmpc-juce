#pragma once

#include "SvgComponent.hpp"

#include <memory>
#include <unordered_map>

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
        };

        std::vector<Press> primaryPresses;
        std::vector<Press> secondaryPresses;
        std::vector<Press> tertiaryPresses;

        int lastBank = -1;

        mpc::Mpc &mpc;
        std::shared_ptr<mpc::hardware::Pad> mpcPad;
        juce::Rectangle<int> rect;
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
        void filesDropped(const juce::StringArray &files, int x,
                          int y) override;
        void paint(juce::Graphics &g) override;

        Pad(juce::Component *commonParentWithShadowToUse,
            const float shadowSizeToUse,
            const std::function<float()> &getScaleToUse, mpc::Mpc &,
            std::shared_ptr<mpc::hardware::Pad>);

        ~Pad() override;
    };

} // namespace vmpc_juce::gui::vector
