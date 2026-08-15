#include "gui/arrangement/ArrangementCatalog.hpp"

#include "gui/arrangement/ArrangementModel.hpp"

#include <algorithm>

namespace vmpc_juce::gui::arrangement
{
    const std::vector<CatalogEntry> &getArrangementCatalog()
    {
        static const std::vector<CatalogEntry> catalog{
            {"lcd-bare", "LCD - bare", "components/lcd_bare", 210, 55},
            {"lcd-mounted", "LCD - mounted", "components/lcd_mounted_trimmed",
             compactMountedLcdReferenceSize.width - 7.f,
             compactMountedLcdReferenceSize.height - 4.f},
            {"lcd-mounted-functions", "LCD - mounted + function buttons",
             "components/display_and_f_keys_trimmed",
             compactDisplayReferenceSize.width - 7.f,
             compactDisplayReferenceSize.height - 4.f},
            {"function-buttons", "Function buttons", "f_keys", 180, 25},
            {"function-buttons-compact", "Function buttons - compact",
             "components/f_keys_unlabelled_trimmed", 180, 10},
            {"main-open", "Main Screen + Open Window",
             "main_screen_and_open_window", 90, 21},
            {"main-open-vertical", "Main Screen + Open Window - vertical",
             "components/main_screen_and_open_window_vertical", 45, 42},
            {"num-pad", "Num pad", "components/num_keys_trimmed", 85, 84},
            {"data-wheel", "DATA wheel",
             "components/data_wheel_unlabelled_trimmed", 72, 80},
            {"note-variation", "Note Variation + After/Assign",
             "components/note_variation_slider", 51, 130},
            {"tap-tempo", "Tap Tempo / Note Repeat",
             "components/tap_tempo_note_repeat", 51, 30},
            {"undo-erase", "Undo Seq + Erase",
             "components/undo_seq_erase_trimmed", 58, 31},
            {"cursor", "Cursor", "components/cursor", 48, 48},
            {"cursor-compact", "Cursor - compact", "components/cursor_compact",
             48, 31},
            {"locate", "Locate", "components/locate_group_trimmed", 179, 28},
            {"transport-horizontal", "Transport - horizontal",
             "components/transport_keys_trimmed", 179, 30},
            {"transport-vertical", "Transport - vertical",
             "components/transport_keys_vertical", 33, 150},
            {"levels", "Full Level + 16 Levels",
             "components/full_level_16_levels", 69, 36},
            {"levels-compact", "Full Level + 16 Levels - compact",
             "components/full_level_16_levels_compact", 69, 23},
            {"sequence-mute", "Next Seq + Track Mute",
             "components/next_seq_track_mute", 69, 23},
            {"pads-banks", "Pads + Pad Bank",
             "components/pads_with_banks_trimmed", 180, 224},
            {"pads", "Pads - compact", "components/pads_trimmed", 180, 188},
            {"gain-volume", "Rec Gain + Main Volume",
             "components/rec_gain_main_volume", 84, 48},
            {"gain-volume-compact", "Rec Gain + Main Volume - compact",
             "components/rec_gain_main_volume_compact", 84, 38},
        };
        return catalog;
    }

    const CatalogEntry *findCatalogEntry(const std::string &id)
    {
        const auto &catalog = getArrangementCatalog();
        const auto found = std::find_if(catalog.begin(), catalog.end(),
                                        [&id](const auto &entry)
                                        {
                                            return id == entry.id;
                                        });
        return found == catalog.end() ? nullptr : &*found;
    }

    bool documentUsesKnownCatalogEntries(const ArrangementDocument &document,
                                         std::string &errorMessage)
    {
        for (const auto &node : document.nodes)
        {
            if (findCatalogEntry(node.catalogId) == nullptr)
            {
                errorMessage =
                    "Unknown arrangement component: " + node.catalogId;
                return false;
            }
        }
        errorMessage.clear();
        return true;
    }
} // namespace vmpc_juce::gui::arrangement
