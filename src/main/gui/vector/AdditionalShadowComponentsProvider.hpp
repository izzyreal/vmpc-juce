#pragma once

#include <vector>

namespace vmpc_juce::gui::vector
{

    class SvgComponent;

    class AdditionalShadowComponentsProvider
    {
    public:
        virtual ~AdditionalShadowComponentsProvider() = default;
        virtual std::vector<SvgComponent *> getAdditionalShadowComponents() = 0;
    };

} // namespace vmpc_juce::gui::vector
