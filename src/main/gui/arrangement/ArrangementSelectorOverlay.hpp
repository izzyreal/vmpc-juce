#pragma once

#include "gui/arrangement/ArrangementModel.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <memory>

namespace vmpc_juce::gui::arrangement
{
    class ArrangementSelectorOverlay final : public juce::Component
    {
    public:
        using FontAtScale = std::function<juce::Font &(float)>;

        ArrangementSelectorOverlay(const ArrangementSetup &setup,
                                   std::size_t selectedSlot,
                                   FontAtScale mainFontAtScale,
                                   FontAtScale faceplateFontAtScale,
                                   std::function<void(std::size_t)> selectSlot,
                                   std::function<void()> close);
        ~ArrangementSelectorOverlay() override;

        void paint(juce::Graphics &) override;
        void resized() override;
        void mouseUp(const juce::MouseEvent &) override;

    private:
        class Thumbnail;
        std::array<std::unique_ptr<Thumbnail>, ArrangementSetup::slotCount>
            thumbnails;
        std::function<void()> close;
    };
} // namespace vmpc_juce::gui::arrangement
