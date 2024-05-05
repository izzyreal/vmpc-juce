#include "VmpcParameter.h"

const int VERSION_HINT = 1;

VmpcParameter::VmpcParameter(mpc::hardware::HwComponent &componentToUse)
: juce::AudioParameterBool(
        juce::ParameterID(componentToUse.getLabel(), VERSION_HINT),
        componentToUse.getLabel(),
        false,
        juce::AudioParameterBoolAttributes()), component(componentToUse)
{
}
