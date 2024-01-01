#include "VmpcParameters.h"

#include "Mpc.hpp"
#include "hardware/Hardware.hpp"
#include "hardware/HwPad.hpp"

const int VERSION_HINT = 1;

VmpcParameters::VmpcParameters(mpc::Mpc& mpc)
{
    listener = std::make_unique<VmpcParameterListener>(mpc);

    parameters.push_back(
            new juce::AudioParameterInt(
                    juce::ParameterID("pad-1", VERSION_HINT),
                    "Pad 1",
                    0, 127, 0));

    parameters.back()->addListener(listener.get());
}

const std::vector<juce::AudioProcessorParameter*>& VmpcParameters::getParameters()
{
    return parameters;
}

VmpcParameterListener::VmpcParameterListener(mpc::Mpc& mpcToUse) : mpc(mpcToUse)
{
}

void VmpcParameterListener::parameterValueChanged(int parameterIndex, float newValue)
{
    const int newIntValue = static_cast<int>(newValue);
    
    if (parameterIndex == 0)
    {
        if (newIntValue == 0)
        {
            mpc.getHardware()->getPad(0)->release();
        }
        else
        {
            mpc.getHardware()->getPad(0)->push(newIntValue);
        }
    }
}

void VmpcParameterListener::parameterGestureChanged(int parameterIndex, bool gestureIsStarting)
{

}
