#pragma once

#include "SvgComponent.hpp"

#include <Observer.hpp>

#include <memory>

namespace mpc { class Mpc; }

namespace mpc::hardware {
    class HwPad;
}

namespace vmpc_juce::gui::vector {

class Pad
        : public SvgComponent,
          public juce::Timer,
          public juce::FileDragAndDropTarget,
          public mpc::Observer
{

private:
    mpc::Mpc &mpc;
    std::weak_ptr<mpc::hardware::HwPad> pad;
    juce::Rectangle<int> rect;
    SvgComponent *glowSvg = nullptr;

    bool fading = false;

    int getVelo(int veloY);
    void loadFile(const juce::String path, bool shouldBeConverted, std::string screenToReturnTo);

public:
    void resized() override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void timerCallback() override;
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;

    void update(mpc::Observable *o, mpc::Message message) override;

    Pad(juce::Component *commonParentWithShadowToUse, const float shadowSizeToUse, const std::function<float()> &getScaleToUse, mpc::Mpc &, std::weak_ptr<mpc::hardware::HwPad>);
    ~Pad() override;
};

} // namespace vmpc_juce::gui::vector
