#include "VmpcJuceRequiredResourceIntegrity.hpp"

#include "VmpcJuceRequiredResources.hpp"

using namespace vmpc_juce;

std::vector<MissingRequiredResource> VmpcJuceRequiredResourceIntegrity::check()
{
    std::vector<MissingRequiredResource> missing;

    for (const auto path : required_resources::paths)
    {
        const std::string logicalPath(path);
        if (VmpcJuceResourceUtil::resourceExists(logicalPath))
        {
            continue;
        }

        missing.push_back(MissingRequiredResource{
            logicalPath, VmpcJuceResourceUtil::resolveResourcePath(logicalPath)});
    }

    return missing;
}
