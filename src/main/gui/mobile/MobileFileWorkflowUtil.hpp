#pragma once

#include <juce_core/juce_core.h>

#include <algorithm>

namespace vmpc_juce::gui::mobile
{
    inline bool hasSupportedMpcFileExtension(const juce::String &name)
    {
        static const juce::StringArray extensions{".wav", ".snd", ".aps",
                                                  ".pgm", ".all", ".mid"};
        const auto lower = name.toLowerCase();
        return std::any_of(extensions.begin(), extensions.end(),
                           [&](const auto &extension)
                           {
                               return lower.endsWith(extension);
                           });
    }

    inline juce::String
    makeUniqueDocumentName(const juce::String &requestedName,
                           const juce::StringArray &existingNames)
    {
        if (!existingNames.contains(requestedName, true))
        {
            return requestedName;
        }

        for (int suffix = 2;; ++suffix)
        {
            const auto candidate =
                requestedName + " (" + juce::String(suffix) + ")";
            if (!existingNames.contains(candidate, true))
            {
                return candidate;
            }
        }
    }
} // namespace vmpc_juce::gui::mobile
