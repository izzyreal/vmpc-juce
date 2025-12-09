#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::utils
{
    template <class ComponentClass>
    static std::vector<ComponentClass *>
    findChildComponentsOfClass(juce::Component *parent)
    {
        std::vector<ComponentClass *> matches;

        for (int i = 0; i < parent->getNumChildComponents(); ++i)
        {
            auto *childComp = parent->getChildComponent(i);

            if (auto *c = dynamic_cast<ComponentClass *>(childComp))
            {
                matches.push_back(c);
            }

            auto childMatches =
                findChildComponentsOfClass<ComponentClass>(childComp);
            matches.insert(matches.end(), childMatches.begin(),
                           childMatches.end());
        }

        return matches;
    }

    template <class ComponentClass>
    static ComponentClass *
    findFirstChildComponentOfClass(juce::Component *parent)
    {
        if (auto results = findChildComponentsOfClass<ComponentClass>(parent);
            results.empty())
        {
            return nullptr;
        }
        else
        {
            return results.front();
        }
    }

} // namespace vmpc_juce::utils