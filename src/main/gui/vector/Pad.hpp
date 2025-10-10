#pragma once

#include "SvgComponent.hpp"

#include <memory>

namespace mpc { class Mpc; }

namespace mpc::hardware2 {
    class Pad;
}

namespace vmpc_juce::gui::vector {

class Pad : public SvgComponent, public juce::FileDragAndDropTarget
{

private:
    mpc::Mpc &mpc;
    std::shared_ptr<mpc::hardware2::Pad> mpcPad;
    juce::Rectangle<int> rect;
    SvgComponent *glowSvg = nullptr;
    std::optional<int> lastProcessedVelo = std::nullopt;

    bool fading = false;
    int16_t timerDivisionCounter = 0;

    int getVelo(int veloY);
    void loadFile(const juce::String path, bool shouldBeConverted, std::string screenToReturnTo);

public:
    void resized() override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void sharedTimerCallback();
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;

    Pad(juce::Component *commonParentWithShadowToUse,
        const float shadowSizeToUse,
        const std::function<float()> &getScaleToUse,
        mpc::Mpc &, std::shared_ptr<mpc::hardware2::Pad>);

    ~Pad() override;
};

} // namespace vmpc_juce::gui::vector
