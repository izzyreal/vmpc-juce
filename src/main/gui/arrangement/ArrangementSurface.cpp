#include "gui/arrangement/ArrangementSurface.hpp"

#include "VmpcJuceResourceUtil.hpp"
#include "gui/arrangement/ArrangementCatalog.hpp"
#include "gui/vector/Constants.hpp"
#include "gui/vector/Node.hpp"
#include "gui/vector/ViewUtil.hpp"

#include <Mpc.hpp>
#include <nlohmann/json.hpp>

#include <cmath>

namespace vmpc_juce::gui::arrangement
{
    struct ArrangementSurface::RenderedItem
    {
        gui::vector::node root;
        float renderedScale = 1.f;
        std::vector<juce::Component *> components;
        std::function<float()> getScale;
        std::function<juce::Font &()> getMainFont;
        std::function<juce::Font &()> getFaceplateFont;
        std::function<juce::Font &()> getKeyTooltipFont;

        ~RenderedItem()
        {
            for (auto *component : components)
            {
                delete component;
            }
        }
    };

    ArrangementSurface::ArrangementSurface(
        mpc::Mpc &mpcToUse, const ArrangementDocument &documentToUse,
        FontAtScale mainFontAtScaleToUse, FontAtScale faceplateFontAtScaleToUse,
        FontAtScale keyTooltipFontAtScaleToUse,
        juce::Component *tooltipOverlayToUse, std::string &errorMessage)
        : mpc(mpcToUse), document(documentToUse),
          mainFontAtScale(std::move(mainFontAtScaleToUse)),
          faceplateFontAtScale(std::move(faceplateFontAtScaleToUse)),
          keyTooltipFontAtScale(std::move(keyTooltipFontAtScaleToUse)),
          tooltipOverlay(tooltipOverlayToUse)
    {
        setInterceptsMouseClicks(false, true);
        if (!documentUsesKnownCatalogEntries(document, errorMessage))
        {
            return;
        }

        for (const auto &node : document.nodes)
        {
            if (!addItem(node.catalogId, errorMessage))
            {
                return;
            }
        }
        errorMessage.clear();
    }

    ArrangementSurface::~ArrangementSurface()
    {
        items.clear();
        for (auto *listener : mouseListeners)
        {
            delete listener;
        }
    }

    bool ArrangementSurface::addItem(const std::string &catalogId,
                                     std::string &errorMessage)
    {
        const auto *entry = findCatalogEntry(catalogId);
        if (entry == nullptr)
        {
            errorMessage = "Unknown arrangement component: " + catalogId;
            return false;
        }
        try
        {
            const auto path =
                std::string("json/") + entry->resourceName + ".json";
            const auto data = VmpcJuceResourceUtil::getResourceData(path);
            if (data.empty())
            {
                throw std::runtime_error("missing " + path);
            }

            auto item = std::make_unique<RenderedItem>();
            item->root = nlohmann::json::parse(data).get<gui::vector::node>();
            auto *stableItem = item.get();
            item->getScale = [stableItem]
            {
                return stableItem->renderedScale;
            };
            item->getMainFont = [this, stableItem]() -> juce::Font &
            {
                return mainFontAtScale(stableItem->renderedScale);
            };
            item->getFaceplateFont = [this, stableItem]() -> juce::Font &
            {
                return faceplateFontAtScale(stableItem->renderedScale);
            };
            item->getKeyTooltipFont = [this, stableItem]() -> juce::Font &
            {
                return keyTooltipFontAtScale(stableItem->renderedScale);
            };
            gui::vector::ViewUtil::createComponent(
                mpc, item->root, item->components, this, item->getScale,
                item->getMainFont, item->getFaceplateFont,
                item->getKeyTooltipFont, mouseListeners, tooltipOverlay);
            if (item->components.empty())
            {
                throw std::runtime_error("component has no renderable root: " +
                                         catalogId);
            }
            items.push_back(std::move(item));
            return true;
        }
        catch (const std::exception &error)
        {
            errorMessage =
                "Could not create " + catalogId + ": " + error.what();
            return false;
        }
    }

    void ArrangementSurface::paint(juce::Graphics &g)
    {
        g.fillAll(gui::vector::Constants::chassisColour);
    }

    void ArrangementSurface::resized()
    {
        rebuildGeometry();
    }

    void ArrangementSurface::rebuildGeometry()
    {
        const LogicalSize target{static_cast<float>(getWidth()),
                                 static_cast<float>(getHeight())};
        const auto layout = computeResponsiveLayout(document, target);
        for (std::size_t nodeIndex = 0; nodeIndex < document.nodes.size();
             ++nodeIndex)
        {
            const auto &geometry = layout.nodes[nodeIndex].geometry;
            auto &item = *items[nodeIndex];
            item.renderedScale = geometry.scale;
            item.components.front()->setBounds(
                juce::roundToInt(geometry.position.x),
                juce::roundToInt(geometry.position.y),
                std::max(1, juce::roundToInt(geometry.size.width)),
                std::max(1, juce::roundToInt(geometry.size.height)));
        }
    }
} // namespace vmpc_juce::gui::arrangement
