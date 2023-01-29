#include "VmpcTooltipComponent.hpp"

#include "controls/Controls.hpp"
#include "controls/KbMapping.hpp"

VmpcTooltipComponent::VmpcTooltipComponent(
        mpc::Mpc &_mpc,
        std::shared_ptr<mpc::hardware::HwComponent> _mpcHardwareComponent)
        : Button(_mpcHardwareComponent->getLabel()),
        mpc(_mpc), mpcHardwareComponent(_mpcHardwareComponent)
{
    setWantsKeyboardFocus(false);
}

void VmpcTooltipComponent::paintButton(juce::Graphics &, bool, bool)
{
    // Nothing to paint. The buttons are part of the background image.
}

void VmpcTooltipComponent::mouseMove(const juce::MouseEvent &event)
{
    if (event.mods.isAnyModifierKeyDown() && !event.mods.isShiftDown() && getTooltip() == "")
    {
        auto label = mpcHardwareComponent->getLabel();
        auto kbMapping = mpc.getControls()->getKbMapping().lock();
        auto tooltipText = kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel(label));

        if (std::dynamic_pointer_cast<DummyDataWheelHwComponent>(mpcHardwareComponent))
        {
            tooltipText =
                    kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel("datawheel-down")) + "/" +
                    kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel("datawheel-up"));
        }

        tooltipText = "Key: " + tooltipText;
        setTooltip(tooltipText);
    }
}

void VmpcTooltipComponent::mouseExit(const juce::MouseEvent &)
{
    setTooltip("");
}
