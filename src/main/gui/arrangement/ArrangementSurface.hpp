#pragma once

#include "gui/arrangement/ArrangementModel.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mpc
{
    class Mpc;
}

namespace vmpc_juce::gui::arrangement
{
    class ArrangementSurface final : public juce::Component
    {
    public:
        using FontAtScale = std::function<juce::Font &(float)>;

        ArrangementSurface(mpc::Mpc &mpc, const ArrangementDocument &document,
                           FontAtScale mainFontAtScale,
                           FontAtScale faceplateFontAtScale,
                           FontAtScale keyTooltipFontAtScale,
                           juce::Component *tooltipOverlay,
                           std::string &errorMessage);
        ~ArrangementSurface() override;

        void paint(juce::Graphics &) override;
        void resized() override;

    private:
        struct RenderedItem;
        bool addItem(const std::string &catalogId, LogicalPoint position,
                     float scale, std::string &errorMessage);
        void rebuildGeometry();

        mpc::Mpc &mpc;
        ArrangementDocument document;
        FontAtScale mainFontAtScale;
        FontAtScale faceplateFontAtScale;
        FontAtScale keyTooltipFontAtScale;
        juce::Component *tooltipOverlay = nullptr;
        std::vector<std::unique_ptr<RenderedItem>> items;
        std::vector<juce::MouseListener *> mouseListeners;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementSurface)
    };
} // namespace vmpc_juce::gui::arrangement
