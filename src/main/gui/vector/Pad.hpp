#pragma once

#include "SvgComponent.hpp"

#include <memory>
#include <unordered_map>

namespace mpc { class Mpc; }

namespace mpc::hardware2 {
    class Pad;
}

namespace vmpc_juce::gui::vector {

class Pad : public SvgComponent, public juce::FileDragAndDropTarget
{

private:
    struct Press {
        int padIndexWithBank;
        float alpha;
        bool isPhysical;
    };

    std::vector<Press> primaryPresses;
    std::vector<Press> secondaryPresses;
    std::vector<Press> tertiaryPresses;

    int lastBank = -1;

    mpc::Mpc &mpc;
    std::shared_ptr<mpc::hardware2::Pad> mpcPad;
    juce::Rectangle<int> rect;
    SvgComponent *glowSvg = nullptr;
    int16_t timerDivisionCounter = 0;
    std::optional<int> pressedBank = std::nullopt;

    int getVelo(int veloY);
    void loadFile(const juce::String path, bool shouldBeConverted, std::string screenToReturnTo);

public:
    void resized() override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void sharedTimerCallback();
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;
    void paint(juce::Graphics& g) override;

    Pad(juce::Component *commonParentWithShadowToUse,
        const float shadowSizeToUse,
        const std::function<float()> &getScaleToUse,
        mpc::Mpc &, std::shared_ptr<mpc::hardware2::Pad>);

    ~Pad() override;
};

} // namespace vmpc_juce::gui::vector
