#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::mobile
{
    // Both rectangles and the insets are in JUCE logical screen coordinates.
    // Intersect instead of padding the editor: a host may already have placed
    // it inside the safe area.
    inline juce::Rectangle<int>
    getSafeEditorScreenBounds(const juce::Rectangle<int> editorBounds,
                              const juce::Rectangle<int> displayBounds,
                              const juce::BorderSize<int> safeAreaInsets)
    {
        return editorBounds.getIntersection(
            safeAreaInsets.subtractedFrom(displayBounds));
    }
} // namespace vmpc_juce::gui::mobile
