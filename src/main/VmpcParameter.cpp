#include "VmpcParameter.h"

const int VERSION_HINT = 1;

VmpcParameter::VmpcParameter(mpc::hardware::HwComponent &componentToUse)
: juce::AudioParameterInt(
        juce::ParameterID(componentToUse.getLabel(), VERSION_HINT),
        componentToUse.getLabel(),
        0, 127, 0), component(componentToUse)
{
}
