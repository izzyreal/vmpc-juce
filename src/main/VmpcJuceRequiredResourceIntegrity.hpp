#pragma once

#include "VmpcJuceResourceUtil.hpp"

#include <vector>

namespace vmpc_juce
{
    class VmpcJuceRequiredResourceIntegrity
    {
    public:
        static std::vector<MissingRequiredResource> check();
    };
} // namespace vmpc_juce
