#include "gui/arrangement/ArrangementSelectorOverlay.hpp"

#include "VmpcJuceResourceUtil.hpp"
#include "gui/arrangement/ArrangementCatalog.hpp"
#include "gui/arrangement/PreviewViewUtil.hpp"
#include "gui/vector/Constants.hpp"
#include "gui/vector/Node.hpp"

#include <nlohmann/json.hpp>

namespace vmpc_juce::gui::arrangement
{
    namespace
    {
        constexpr auto thumbnailInset = 8;
        constexpr float portraitAspect = 390.f / 844.f;

        class PreviewSurface final : public juce::Component
        {
        public:
            using FontAtScale = ArrangementSelectorOverlay::FontAtScale;

            PreviewSurface(const ArrangementDocument &documentToUse,
                           FontAtScale mainFontAtScaleToUse,
                           FontAtScale faceplateFontAtScaleToUse)
                : document(documentToUse),
                  mainFontAtScale(std::move(mainFontAtScaleToUse)),
                  faceplateFontAtScale(std::move(faceplateFontAtScaleToUse))
            {
                setInterceptsMouseClicks(false, false);
                for (const auto &node : document.nodes)
                {
                    if (!node.isGroup())
                    {
                        addItem(node.catalogId, {}, 1.f);
                    }
                    else
                    {
                        for (const auto &child : node.children)
                        {
                            addItem(child.catalogId, child.position,
                                    child.scale);
                        }
                    }
                }
            }

            ~PreviewSurface() override
            {
                for (auto &item : items)
                {
                    for (auto *component : item->components)
                    {
                        delete component;
                    }
                }
            }

            void paint(juce::Graphics &g) override
            {
                g.fillAll(gui::vector::Constants::chassisColour);
            }

            void resized() override
            {
                const LogicalSize target{static_cast<float>(getWidth()),
                                         static_cast<float>(getHeight())};
                const auto layout = computeResponsiveLayout(document, target);
                std::size_t itemIndex = 0;
                for (std::size_t nodeIndex = 0;
                     nodeIndex < document.nodes.size(); ++nodeIndex)
                {
                    const auto &node = document.nodes[nodeIndex];
                    const auto &geometry = layout.nodes[nodeIndex].geometry;
                    if (!node.isGroup())
                    {
                        setItemBounds(*items[itemIndex++], geometry.position,
                                      geometry.size, geometry.scale);
                        continue;
                    }
                    for (const auto &child : node.children)
                    {
                        auto &item = *items[itemIndex++];
                        const auto scale = geometry.scale * child.scale;
                        setItemBounds(item,
                                      {geometry.position.x +
                                           child.position.x * geometry.scale,
                                       geometry.position.y +
                                           child.position.y * geometry.scale},
                                      {child.referenceSize.width * scale,
                                       child.referenceSize.height * scale},
                                      scale);
                    }
                }
            }

        private:
            struct Item
            {
                gui::vector::node root;
                float renderedScale = 1.f;
                std::vector<juce::Component *> components;
                std::function<float()> getScale;
                std::function<juce::Font &()> getMainFont;
                std::function<juce::Font &()> getFaceplateFont;
            };

            void addItem(const std::string &catalogId, LogicalPoint, float)
            {
                auto item = std::make_unique<Item>();
                const auto *entry = findCatalogEntry(catalogId);
                if (entry == nullptr)
                {
                    items.push_back(std::move(item));
                    return;
                }
                try
                {
                    const auto data = VmpcJuceResourceUtil::getResourceData(
                        std::string("json/") + entry->resourceName + ".json");
                    item->root =
                        nlohmann::json::parse(data).get<gui::vector::node>();
                    auto *stableItem = item.get();
                    item->getScale = [stableItem]
                    {
                        return stableItem->renderedScale;
                    };
                    item->getMainFont = [this, stableItem]() -> juce::Font &
                    {
                        return mainFontAtScale(stableItem->renderedScale);
                    };
                    item->getFaceplateFont = [this,
                                              stableItem]() -> juce::Font &
                    {
                        return faceplateFontAtScale(stableItem->renderedScale);
                    };
                    PreviewViewUtil::createComponent(
                        item->root, item->components, this, item->getScale,
                        item->getMainFont, item->getFaceplateFont);
                }
                catch (...)
                {
                }
                items.push_back(std::move(item));
            }

            static void setItemBounds(Item &item, const LogicalPoint position,
                                      const LogicalSize size, const float scale)
            {
                item.renderedScale = scale;
                if (!item.components.empty())
                {
                    item.components.front()->setBounds(
                        juce::roundToInt(position.x),
                        juce::roundToInt(position.y),
                        std::max(1, juce::roundToInt(size.width)),
                        std::max(1, juce::roundToInt(size.height)));
                }
            }

            ArrangementDocument document;
            FontAtScale mainFontAtScale;
            FontAtScale faceplateFontAtScale;
            std::vector<std::unique_ptr<Item>> items;
        };
    } // namespace

    class ArrangementSelectorOverlay::Thumbnail final : public juce::Component
    {
    public:
        Thumbnail(const std::size_t indexToUse,
                  const std::optional<ArrangementSlot> &slotToUse,
                  const bool selectedToUse, FontAtScale mainFontAtScale,
                  FontAtScale faceplateFontAtScale,
                  std::function<void(std::size_t)> selectSlotToUse)
            : index(indexToUse), slot(slotToUse), selected(selectedToUse),
              selectSlot(std::move(selectSlotToUse))
        {
            if (slot.has_value() && !slot->arrangement.nodes.empty())
            {
                preview = std::make_unique<PreviewSurface>(
                    slot->arrangement, std::move(mainFontAtScale),
                    std::move(faceplateFontAtScale));
                addAndMakeVisible(*preview);
                setMouseCursor(juce::MouseCursor::PointingHandCursor);
            }
        }

        void paint(juce::Graphics &g) override
        {
            const auto bounds = getLocalBounds().toFloat().reduced(1.f);
            g.setColour(juce::Colour(0xff303634));
            g.fillRoundedRectangle(bounds, 5.f);
            g.setColour(selected ? juce::Colour(0xff67b8de)
                                 : juce::Colour(0xff8b9692));
            g.drawRoundedRectangle(bounds, 5.f, selected ? 3.f : 1.f);
            if (preview == nullptr)
            {
                g.setColour(juce::Colour(0xff8b9692));
                g.setFont(12.f);
                g.drawFittedText("Empty", getLocalBounds().reduced(6),
                                 juce::Justification::centred, 1);
            }
        }

        void resized() override
        {
            if (preview == nullptr || !slot.has_value())
            {
                return;
            }
            auto area = getLocalBounds().reduced(thumbnailInset);
            const auto aspect = slot->orientation == Orientation::portrait
                                    ? portraitAspect
                                    : 1.f / portraitAspect;
            auto width = static_cast<float>(area.getWidth());
            auto height = width / aspect;
            if (height > static_cast<float>(area.getHeight()))
            {
                height = static_cast<float>(area.getHeight());
                width = height * aspect;
            }
            preview->setBounds(area.getCentreX() - juce::roundToInt(width) / 2,
                               area.getCentreY() - juce::roundToInt(height) / 2,
                               std::max(1, juce::roundToInt(width)),
                               std::max(1, juce::roundToInt(height)));
        }

        void mouseUp(const juce::MouseEvent &) override
        {
            if (preview != nullptr)
            {
                selectSlot(index);
            }
        }

    private:
        std::size_t index;
        std::optional<ArrangementSlot> slot;
        bool selected;
        std::function<void(std::size_t)> selectSlot;
        std::unique_ptr<PreviewSurface> preview;
    };

    ArrangementSelectorOverlay::ArrangementSelectorOverlay(
        const ArrangementSetup &setup, const std::size_t selectedSlot,
        FontAtScale mainFontAtScale, FontAtScale faceplateFontAtScale,
        std::function<void(std::size_t)> selectSlot,
        std::function<void()> closeToUse)
        : close(std::move(closeToUse))
    {
        setAlwaysOnTop(true);
        for (std::size_t i = 0; i < thumbnails.size(); ++i)
        {
            thumbnails[i] = std::make_unique<Thumbnail>(
                i, setup.slots[i], i == selectedSlot, mainFontAtScale,
                faceplateFontAtScale, selectSlot);
            addAndMakeVisible(*thumbnails[i]);
        }
    }

    ArrangementSelectorOverlay::~ArrangementSelectorOverlay() = default;

    void ArrangementSelectorOverlay::paint(juce::Graphics &g)
    {
        g.fillAll(juce::Colours::black.withAlpha(0.72f));
    }

    void ArrangementSelectorOverlay::resized()
    {
        auto area = getLocalBounds().reduced(
            std::max(12, juce::roundToInt(getWidth() * 0.035f)));
        const auto gap = std::max(6, juce::roundToInt(getWidth() * 0.012f));
        const auto width = std::max(1, (area.getWidth() - gap * 4) / 5);
        const auto previewWidth = std::max(1, width - thumbnailInset * 2);
        const auto heightNeededForPortraitPreview =
            juce::roundToInt(static_cast<float>(previewWidth) /
                             portraitAspect) +
            thumbnailInset * 2;
        const auto height =
            std::min(area.getHeight(), heightNeededForPortraitPreview);
        area = area.withSizeKeepingCentre(area.getWidth(), height);
        for (auto &thumbnail : thumbnails)
        {
            thumbnail->setBounds(area.removeFromLeft(width));
            area.removeFromLeft(gap);
        }
    }

    void ArrangementSelectorOverlay::mouseUp(const juce::MouseEvent &event)
    {
        if (event.eventComponent == this)
        {
            close();
        }
    }
} // namespace vmpc_juce::gui::arrangement
