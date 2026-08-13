#pragma once

#include <string>
#include <vector>

namespace vmpc_juce::gui::arrangement
{
    struct CatalogEntry
    {
        const char *id;
        const char *title;
        const char *resourceName;
        float referenceWidth;
        float referenceHeight;
    };

    const std::vector<CatalogEntry> &getArrangementCatalog();
    const CatalogEntry *findCatalogEntry(const std::string &id);
    bool documentUsesKnownCatalogEntries(const struct ArrangementDocument &,
                                         std::string &errorMessage);
} // namespace vmpc_juce::gui::arrangement
