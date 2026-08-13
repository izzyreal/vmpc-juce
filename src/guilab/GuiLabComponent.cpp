#include "GuiLabComponent.hpp"

#include "PreviewViewUtil.hpp"
#include "VmpcJuceResourceUtil.hpp"
#include "gui/vector/Constants.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <string>

using namespace vmpc_juce::guilab;
using vmpc_juce::gui::vector::Constants;

namespace
{
    constexpr std::array<CatalogEntry, 22> catalog{{
        {"lcd-bare", "LCD - bare", "lcd_bare", 210, 55},
        {"lcd-mounted", "LCD - mounted", "lcd_mounted_lab",
         compactMountedLcdReferenceSize.width,
         compactMountedLcdReferenceSize.height},
        {"lcd-mounted-functions", "LCD - mounted + function buttons",
         "display_and_f_keys_compact", compactDisplayReferenceSize.width,
         compactDisplayReferenceSize.height},
        {"function-buttons", "Function buttons", "f_keys", 180, 25},
        {"main-open", "Main Screen + Open Window",
         "main_screen_and_open_window", 102, 24},
        {"num-pad", "Num pad", "num_keys", 102, 84},
        {"data-wheel", "DATA wheel", "data_wheel_unlabelled", 80, 80},
        {"note-variation", "Note Variation + After/Assign",
         "note_variation_slider", 51, 130},
        {"tap-tempo", "Tap Tempo / Note Repeat", "tap_tempo_note_repeat", 51,
         30},
        {"undo-erase", "Undo Seq + Erase", "undo_seq_erase", 60, 31},
        {"cursor", "Cursor", "cursor", 80, 48},
        {"cursor-compact", "Cursor - compact", "cursor_compact", 80, 31},
        {"locate", "Locate", "locate_group_lab", 179, 28},
        {"transport-horizontal", "Transport - horizontal", "transport_keys_lab",
         179, 30},
        {"transport-vertical", "Transport - vertical",
         "transport_keys_vertical", 45, 150},
        {"levels", "Full Level + 16 Levels", "full_level_16_levels", 72, 36},
        {"levels-compact", "Full Level + 16 Levels - compact",
         "full_level_16_levels_compact", 72, 23},
        {"sequence-mute", "Next Seq + Track Mute", "next_seq_track_mute", 72,
         23},
        {"pads-banks", "Pads + Pad Bank", "pads_with_banks", 206, 236},
        {"pads", "Pads - compact", "pads", 206, 200},
        {"gain-volume", "Rec Gain + Main Volume", "rec_gain_main_volume", 84,
         48},
        {"gain-volume-compact", "Rec Gain + Main Volume - compact",
         "rec_gain_main_volume_compact", 84, 38},
    }};

    constexpr auto dragDescriptionPrefix = "component:";

    const CatalogEntry *findCatalogEntry(const std::string &id)
    {
        const auto found = std::find_if(catalog.begin(), catalog.end(),
                                        [&id](const auto &entry)
                                        {
                                            return id == entry.id;
                                        });
        return found == catalog.end() ? nullptr : &*found;
    }

    const CatalogEntry *catalogEntryFromDragDescription(const juce::var &value)
    {
        const auto description = value.toString().toStdString();
        if (description.rfind(dragDescriptionPrefix, 0) != 0)
        {
            return nullptr;
        }

        return findCatalogEntry(description.substr(
            std::char_traits<char>::length(dragDescriptionPrefix)));
    }

    void styleControlLabel(juce::Label &label, const juce::String &text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(12.f, juce::Font::bold));
        label.setColour(juce::Label::textColourId, juce::Colour(0xffaeb7b3));
        label.setJustificationType(juce::Justification::centredLeft);
    }

    void styleComboBox(juce::ComboBox &comboBox)
    {
        comboBox.setColour(juce::ComboBox::backgroundColourId,
                           juce::Colour(0xff343a38));
        comboBox.setColour(juce::ComboBox::textColourId,
                           juce::Colour(0xffedf2ef));
        comboBox.setColour(juce::ComboBox::outlineColourId,
                           juce::Colour(0xff59615e));
        comboBox.setColour(juce::ComboBox::arrowColourId,
                           juce::Colour(0xffaeb7b3));
    }

    juce::Font loadFont(const std::vector<char> &data)
    {
        if (data.empty())
        {
            return juce::Font(12.f);
        }

        return juce::Font(
            juce::Typeface::createSystemTypefaceFor(data.data(), data.size()));
    }
} // namespace

PreviewComponent::PreviewComponent(const CatalogEntry &entry)
    : getScale(
          [this]
          {
              return hardwareScale;
          }),
      getMainFontScaled(
          [this]() -> juce::Font &
          {
              mainFont.setHeight(Constants::BASE_FONT_SIZE * hardwareScale);
              return mainFont;
          }),
      getFaceplateFontScaled(
          [this]() -> juce::Font &
          {
              faceplateFont.setHeight(Constants::BASE_FONT_SIZE *
                                      hardwareScale);
              return faceplateFont;
          })
{
    setInterceptsMouseClicks(false, true);

    try
    {
        mainFontData =
            VmpcJuceResourceUtil::getResourceData("fonts/NeutralSans-Bold.ttf");
        faceplateFontData = VmpcJuceResourceUtil::getResourceData(
            "fonts/mpc2000xl-faceplate-glyphs.ttf");
        mainFont = loadFont(mainFontData);
        faceplateFont = loadFont(faceplateFontData);

        const auto resourcePath =
            std::string("json/") + entry.resourceName + ".json";
        const auto data = VmpcJuceResourceUtil::getResourceData(resourcePath);
        if (data.empty())
        {
            throw std::runtime_error("missing " + resourcePath);
        }

        root = nlohmann::json::parse(data).get<gui::vector::node>();
        PreviewViewUtil::createComponent(root, components, this, getScale,
                                         getMainFontScaled,
                                         getFaceplateFontScaled);
    }
    catch (const std::exception &error)
    {
        errorMessage = error.what();
    }
    catch (...)
    {
        errorMessage = "unknown preview error";
    }
}

PreviewComponent::~PreviewComponent()
{
    for (auto *component : components)
    {
        delete component;
    }
}

void PreviewComponent::paint(juce::Graphics &g)
{
    g.fillAll(Constants::chassisColour);

    if (!errorMessage.empty())
    {
        g.setColour(juce::Colours::darkred);
        g.drawRect(getLocalBounds(), 2);
        g.setFont(13.f);
        g.drawFittedText(errorMessage, getLocalBounds().reduced(8),
                         juce::Justification::centred, 4);
    }
}

void PreviewComponent::resized()
{
    if (!components.empty())
    {
        components.front()->setBounds(getLocalBounds());
    }
}

void PreviewComponent::setHardwareScale(const float newScale)
{
    hardwareScale = newScale;
    resized();
    repaint();
}

class GuiLabComponent::PreviewCard final : public juce::Component
{
public:
    class DragSource final : public juce::Component
    {
    public:
        DragSource(const CatalogEntry &entryToUse,
                   PreviewComponent &previewToUse,
                   std::function<float(const CatalogEntry &)> getDropScaleToUse)
            : entry(entryToUse), preview(previewToUse),
              getDropScale(std::move(getDropScaleToUse))
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }

        void mouseDown(const juce::MouseEvent &) override
        {
            dragStarted = false;
        }

        void mouseDrag(const juce::MouseEvent &event) override
        {
            if (dragStarted || event.getDistanceFromDragStart() < 4)
            {
                return;
            }

            auto *container =
                juce::DragAndDropContainer::findParentDragContainerFor(this);
            if (container == nullptr)
            {
                return;
            }

            dragStarted = true;
            const auto snapshot =
                preview.createComponentSnapshot(preview.getLocalBounds());
            const auto dropScale = std::max(0.01f, getDropScale(entry));
            const auto dragImage = snapshot.rescaled(
                std::max(1, juce::roundToInt(entry.referenceWidth * dropScale)),
                std::max(1,
                         juce::roundToInt(entry.referenceHeight * dropScale)),
                juce::Graphics::highResamplingQuality);
            container->startDragging(
                juce::String(dragDescriptionPrefix) + entry.id, this,
                juce::ScaledImage(dragImage), false, nullptr, &event.source);
        }

    private:
        CatalogEntry entry;
        PreviewComponent &preview;
        std::function<float(const CatalogEntry &)> getDropScale;
        bool dragStarted = false;
    };

    PreviewCard(const CatalogEntry &entryToUse,
                std::function<float(const CatalogEntry &)> getDropScale)
        : entry(entryToUse), preview(entry),
          dragSource(entry, preview, std::move(getDropScale))
    {
        title.setText(entry.title, juce::dontSendNotification);
        title.setFont(juce::Font(15.f, juce::Font::bold));
        title.setColour(juce::Label::textColourId, juce::Colour(0xffe8ecea));
        title.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(title);
        addAndMakeVisible(preview);
        addAndMakeVisible(dragSource);
    }

    void setHardwareScale(const float newScale)
    {
        scale = newScale;
        preview.setHardwareScale(scale);
    }

    int requiredWidth() const
    {
        return std::max(230,
                        juce::roundToInt(entry.referenceWidth * scale) + 24);
    }

    int requiredHeight() const
    {
        return juce::roundToInt(entry.referenceHeight * scale) + 54;
    }

    void paint(juce::Graphics &g) override
    {
        g.setColour(juce::Colour(0xff343a38));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.f);
        g.setColour(juce::Colour(0xff59615e));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.f,
                               1.f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(12);
        title.setBounds(bounds.removeFromTop(26));
        bounds.removeFromTop(4);
        preview.setBounds(bounds.withSizeKeepingCentre(
            juce::roundToInt(entry.referenceWidth * scale),
            juce::roundToInt(entry.referenceHeight * scale)));
        dragSource.setBounds(preview.getBounds());
    }

private:
    CatalogEntry entry;
    float scale = 2.f;
    juce::Label title;
    PreviewComponent preview;
    DragSource dragSource;
};

class GuiLabComponent::ArrangementWorkspace final
    : public juce::Component,
      public juce::DragAndDropTarget
{
public:
    explicit ArrangementWorkspace(const DeviceProfile &deviceToUse)
        : device(deviceToUse)
    {
        setWantsKeyboardFocus(true);
        setFocusContainerType(
            juce::Component::FocusContainerType::focusContainer);
    }

    void setTarget(const DeviceProfile &newDevice,
                   const Orientation newOrientation)
    {
        device = newDevice;
        orientation = newOrientation;
        const auto deviceSize = getEffectiveDeviceSize(device, orientation);

        for (auto &item : items)
        {
            auto &model = item->getModel();
            const auto referenceSize = item->getReferenceSize();
            model.scale =
                constrainItemScale(model.scale, referenceSize, deviceSize);
            const LogicalSize itemSize{referenceSize.width * model.scale,
                                       referenceSize.height * model.scale};
            model.position = constrainItemPosition(model.position, itemSize,
                                                   deviceSize, false);
        }

        resized();
        repaint();
    }

    float getDefaultItemDisplayScale(const CatalogEntry &entry) const
    {
        const auto itemScale = constrainItemScale(
            1.f, {entry.referenceWidth, entry.referenceHeight},
            getEffectiveDeviceSize(device, orientation));
        return zoom * itemScale;
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colour(0xff282e2c));

        const auto deviceSize = getEffectiveDeviceSize(device, orientation);
        const auto description =
            juce::String(device.name) + " — " +
            juce::String(juce::roundToInt(deviceSize.width)) + " × " +
            juce::String(juce::roundToInt(deviceSize.height)) + " pt — " +
            juce::String(juce::roundToInt(zoom * 100.f)) + "%";
        g.setColour(juce::Colour(0xffaeb7b3));
        g.setFont(13.f);
        g.drawText(description, getLocalBounds().removeFromTop(26),
                   juce::Justification::centred);

        g.setColour(Constants::chassisColour);
        g.fillRect(surfaceBounds);
        g.setColour(isDragOverSurface ? juce::Colour(0xff43b3dd)
                                      : juce::Colour(0xff87918e));
        g.drawRect(surfaceBounds, isDragOverSurface ? 3 : 2);
    }

    void resized() override
    {
        auto available = getLocalBounds().reduced(18);
        available.removeFromTop(28);

        const auto deviceSize = getEffectiveDeviceSize(device, orientation);
        const auto horizontalZoom =
            static_cast<float>(available.getWidth()) / deviceSize.width;
        const auto verticalZoom =
            static_cast<float>(available.getHeight()) / deviceSize.height;
        zoom = std::max(0.01f, std::min(horizontalZoom, verticalZoom));

        const auto surfaceWidth =
            std::max(1, juce::roundToInt(deviceSize.width * zoom));
        const auto surfaceHeight =
            std::max(1, juce::roundToInt(deviceSize.height * zoom));
        surfaceBounds =
            available.withSizeKeepingCentre(surfaceWidth, surfaceHeight);

        for (auto &item : items)
        {
            updateItemBounds(*item);
        }
    }

    void mouseDown(const juce::MouseEvent &) override
    {
        selectItem(nullptr);
        grabKeyboardFocus();
    }

    bool keyPressed(const juce::KeyPress &key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            selectItem(nullptr);
            return true;
        }

        if (selectedItem != nullptr && (key == juce::KeyPress::deleteKey ||
                                        key == juce::KeyPress::backspaceKey))
        {
            const auto found =
                std::find_if(items.begin(), items.end(),
                             [this](const auto &item)
                             {
                                 return item.get() == selectedItem;
                             });
            if (found != items.end())
            {
                items.erase(found);
                selectedItem = nullptr;
                repaint();
            }
            return true;
        }

        return false;
    }

    bool isInterestedInDragSource(const SourceDetails &details) override
    {
        return catalogEntryFromDragDescription(details.description) != nullptr;
    }

    void itemDragEnter(const SourceDetails &details) override
    {
        updateDragHighlight(details.localPosition);
    }

    void itemDragMove(const SourceDetails &details) override
    {
        updateDragHighlight(details.localPosition);
    }

    void itemDragExit(const SourceDetails &) override
    {
        isDragOverSurface = false;
        repaint();
    }

    void itemDropped(const SourceDetails &details) override
    {
        isDragOverSurface = false;
        const auto *entry =
            catalogEntryFromDragDescription(details.description);
        if (entry == nullptr || !surfaceBounds.contains(details.localPosition))
        {
            repaint();
            return;
        }

        const auto deviceSize = getEffectiveDeviceSize(device, orientation);
        const LogicalSize referenceSize{entry->referenceWidth,
                                        entry->referenceHeight};
        ArrangementItemModel model;
        model.catalogId = entry->id;
        model.scale = constrainItemScale(1.f, referenceSize, deviceSize);

        const auto logicalDropX =
            static_cast<float>(details.localPosition.x - surfaceBounds.getX()) /
            zoom;
        const auto logicalDropY =
            static_cast<float>(details.localPosition.y - surfaceBounds.getY()) /
            zoom;
        const LogicalSize itemSize{referenceSize.width * model.scale,
                                   referenceSize.height * model.scale};
        const LogicalPoint requested{logicalDropX - itemSize.width * 0.5f,
                                     logicalDropY - itemSize.height * 0.5f};
        const auto shouldSnap =
            !juce::ModifierKeys::getCurrentModifiersRealtime().isAltDown();
        model.position =
            constrainItemPosition(requested, itemSize, deviceSize, shouldSnap);

        auto item = std::make_unique<ArrangementItem>(*this, *entry, model);
        auto *itemPointer = item.get();
        addAndMakeVisible(*itemPointer);
        items.push_back(std::move(item));
        updateItemBounds(*itemPointer);
        selectItem(itemPointer);
        grabKeyboardFocus();
        repaint();
    }

private:
    class ArrangementItem final : public juce::Component
    {
    public:
        class InteractionLayer final : public juce::Component
        {
        public:
            explicit InteractionLayer(ArrangementItem &ownerToUse)
                : owner(ownerToUse)
            {
                setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            }

            void paint(juce::Graphics &g) override
            {
                if (!owner.selected)
                {
                    return;
                }

                g.setColour(juce::Colour(0xff43b3dd));
                g.drawRect(getLocalBounds(), 3);
            }

            void mouseDown(const juce::MouseEvent &event) override
            {
                owner.beginMove(event);
            }

            void mouseDrag(const juce::MouseEvent &event) override
            {
                owner.continueMove(event);
            }

        private:
            ArrangementItem &owner;
        };

        class ResizeHandle final : public juce::Component
        {
        public:
            explicit ResizeHandle(ArrangementItem &ownerToUse)
                : owner(ownerToUse)
            {
                setMouseCursor(
                    juce::MouseCursor::BottomRightCornerResizeCursor);
            }

            void paint(juce::Graphics &g) override
            {
                g.setColour(juce::Colour(0xff43b3dd));
                g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.f),
                                       2.f);
                g.setColour(juce::Colour(0xffedf2ef));
                g.drawLine(4.f, static_cast<float>(getHeight() - 4),
                           static_cast<float>(getWidth() - 4), 4.f, 1.5f);
            }

            void mouseDown(const juce::MouseEvent &event) override
            {
                owner.workspace.selectItem(&owner);
                startScale = owner.model.scale;
                startScreenPosition = event.getScreenPosition();
            }

            void mouseDrag(const juce::MouseEvent &event) override
            {
                const auto delta =
                    event.getScreenPosition() - startScreenPosition;
                const auto reference = owner.getReferenceSize();
                const auto denominator = owner.workspace.zoom *
                                         (reference.width * reference.width +
                                          reference.height * reference.height);
                if (denominator <= 0.f)
                {
                    return;
                }

                const auto projectedDelta =
                    (static_cast<float>(delta.x) * reference.width +
                     static_cast<float>(delta.y) * reference.height) /
                    denominator;
                owner.workspace.resizeItem(owner, startScale + projectedDelta,
                                           !event.mods.isAltDown());
            }

        private:
            ArrangementItem &owner;
            float startScale = 1.f;
            juce::Point<int> startScreenPosition;
        };

        ArrangementItem(ArrangementWorkspace &workspaceToUse,
                        const CatalogEntry &entryToUse,
                        const ArrangementItemModel &modelToUse)
            : workspace(workspaceToUse), entry(entryToUse), model(modelToUse),
              preview(entry), interactionLayer(*this), resizeHandle(*this)
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            preview.setInterceptsMouseClicks(false, false);
            addAndMakeVisible(preview);
            addAndMakeVisible(interactionLayer);
            addAndMakeVisible(resizeHandle);
            resizeHandle.setVisible(false);
        }

        ArrangementItemModel &getModel()
        {
            return model;
        }

        LogicalSize getReferenceSize() const
        {
            return {entry.referenceWidth, entry.referenceHeight};
        }

        void setSelected(const bool shouldBeSelected)
        {
            selected = shouldBeSelected;
            resizeHandle.setVisible(selected);
            interactionLayer.repaint();
            repaint();
        }

        void setPreviewScale(const float scale)
        {
            preview.setHardwareScale(scale);
        }

        void resized() override
        {
            preview.setBounds(getLocalBounds());
            interactionLayer.setBounds(getLocalBounds());
            constexpr int handleSize = 16;
            resizeHandle.setBounds(getWidth() - handleSize,
                                   getHeight() - handleSize, handleSize,
                                   handleSize);
            resizeHandle.toFront(false);
        }

    private:
        void beginMove(const juce::MouseEvent &event)
        {
            workspace.selectItem(this);
            workspace.grabKeyboardFocus();
            startPosition = model.position;
            startScreenPosition = event.getScreenPosition();
        }

        void continueMove(const juce::MouseEvent &event)
        {
            const auto delta = event.getScreenPosition() - startScreenPosition;
            const LogicalPoint requested{
                startPosition.x + static_cast<float>(delta.x) / workspace.zoom,
                startPosition.y + static_cast<float>(delta.y) / workspace.zoom};
            workspace.moveItem(*this, requested, !event.mods.isAltDown());
        }

        ArrangementWorkspace &workspace;
        CatalogEntry entry;
        ArrangementItemModel model;
        PreviewComponent preview;
        InteractionLayer interactionLayer;
        ResizeHandle resizeHandle;
        LogicalPoint startPosition;
        juce::Point<int> startScreenPosition;
        bool selected = false;
    };

    void updateDragHighlight(const juce::Point<int> position)
    {
        const auto shouldHighlight = surfaceBounds.contains(position);
        if (shouldHighlight != isDragOverSurface)
        {
            isDragOverSurface = shouldHighlight;
            repaint();
        }
    }

    void selectItem(ArrangementItem *item)
    {
        if (selectedItem == item)
        {
            return;
        }

        if (selectedItem != nullptr)
        {
            selectedItem->setSelected(false);
        }
        selectedItem = item;
        if (selectedItem != nullptr)
        {
            selectedItem->setSelected(true);
            selectedItem->toFront(false);
        }
    }

    void moveItem(ArrangementItem &item, const LogicalPoint requestedPosition,
                  const bool shouldSnap)
    {
        auto &model = item.getModel();
        const auto referenceSize = item.getReferenceSize();
        const LogicalSize itemSize{referenceSize.width * model.scale,
                                   referenceSize.height * model.scale};
        model.position = constrainItemPosition(
            requestedPosition, itemSize,
            getEffectiveDeviceSize(device, orientation), shouldSnap);
        updateItemBounds(item);
    }

    void resizeItem(ArrangementItem &item, float requestedScale,
                    const bool shouldSnap)
    {
        auto &model = item.getModel();
        const auto referenceSize = item.getReferenceSize();
        const auto deviceSize = getEffectiveDeviceSize(device, orientation);
        if (shouldSnap)
        {
            requestedScale = snapItemScaleToGrid(requestedScale, referenceSize);
        }
        model.scale =
            constrainItemScale(requestedScale, referenceSize, deviceSize);
        const LogicalSize itemSize{referenceSize.width * model.scale,
                                   referenceSize.height * model.scale};
        model.position =
            constrainItemPosition(model.position, itemSize, deviceSize, false);
        updateItemBounds(item);
    }

    void updateItemBounds(ArrangementItem &item)
    {
        const auto &model = item.getModel();
        const auto referenceSize = item.getReferenceSize();
        item.setPreviewScale(zoom * model.scale);
        item.setBounds(
            surfaceBounds.getX() + juce::roundToInt(model.position.x * zoom),
            surfaceBounds.getY() + juce::roundToInt(model.position.y * zoom),
            std::max(
                1, juce::roundToInt(referenceSize.width * model.scale * zoom)),
            std::max(1, juce::roundToInt(referenceSize.height * model.scale *
                                         zoom)));
    }

    DeviceProfile device;
    Orientation orientation = Orientation::portrait;
    float zoom = 1.f;
    juce::Rectangle<int> surfaceBounds;
    std::vector<std::unique_ptr<ArrangementItem>> items;
    ArrangementItem *selectedItem = nullptr;
    bool isDragOverSurface = false;
};

GuiLabComponent::GuiLabComponent()
{
    heading.setText("VMPC2000XL arrangement lab", juce::dontSendNotification);
    heading.setFont(juce::Font(24.f, juce::Font::bold));
    heading.setColour(juce::Label::textColourId, juce::Colour(0xffedf2ef));
    heading.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(heading);

    paletteHeading.setText("Components", juce::dontSendNotification);
    paletteHeading.setFont(juce::Font(17.f, juce::Font::bold));
    paletteHeading.setColour(juce::Label::textColourId,
                             juce::Colour(0xffedf2ef));
    paletteHeading.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(paletteHeading);

    paletteViewport.setViewedComponent(&paletteGallery, false);
    paletteViewport.setScrollBarsShown(true, false);
    paletteViewport.setColour(juce::ScrollBar::thumbColourId,
                              juce::Colour(0xff68716e));
    addAndMakeVisible(paletteViewport);

    cards.reserve(catalog.size());
    for (const auto &entry : catalog)
    {
        auto card = std::make_unique<PreviewCard>(
            entry,
            [this](const CatalogEntry &catalogEntry)
            {
                return workspace != nullptr
                           ? workspace->getDefaultItemDisplayScale(catalogEntry)
                           : 1.f;
            });
        paletteGallery.addAndMakeVisible(*card);
        cards.push_back(std::move(card));
    }

    const auto &profiles = getDeviceProfiles();
    const auto defaultDevice =
        std::find_if(profiles.begin(), profiles.end(),
                     [](const auto &profile)
                     {
                         return std::string(profile.id) == "iphone-12-13-pro";
                     });
    workspace = std::make_unique<ArrangementWorkspace>(
        defaultDevice != profiles.end() ? *defaultDevice : profiles.front());
    addAndMakeVisible(*workspace);
    configureControls();
}

GuiLabComponent::~GuiLabComponent()
{
    paletteViewport.setViewedComponent(nullptr, false);
}

void GuiLabComponent::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff202523));
}

void GuiLabComponent::resized()
{
    auto bounds = getLocalBounds().reduced(18);
    heading.setBounds(bounds.removeFromTop(38));
    bounds.removeFromTop(6);

    auto controls = bounds.removeFromTop(50);
    constexpr int controlGap = 12;
    auto brandBounds = controls.removeFromLeft(150);
    controls.removeFromLeft(controlGap);
    auto orientationBounds = controls.removeFromRight(150);
    controls.removeFromRight(controlGap);
    auto deviceBounds = controls;

    const auto placeControl =
        [](juce::Label &label, juce::ComboBox &combo, juce::Rectangle<int> area)
    {
        label.setBounds(area.removeFromTop(16));
        combo.setBounds(area.removeFromTop(30));
    };
    placeControl(brandLabel, brandSelector, brandBounds);
    placeControl(deviceLabel, deviceSelector, deviceBounds);
    placeControl(orientationLabel, orientationSelector, orientationBounds);

    bounds.removeFromTop(10);
    constexpr int paneGap = 14;
    const auto paletteWidth =
        std::clamp(juce::roundToInt(bounds.getWidth() * 0.3f), 280, 390);
    auto paletteBounds = bounds.removeFromLeft(paletteWidth);
    bounds.removeFromLeft(paneGap);
    paletteHeading.setBounds(paletteBounds.removeFromTop(28));
    paletteViewport.setBounds(paletteBounds);
    workspace->setBounds(bounds);
    layoutPalette();
}

void GuiLabComponent::configureControls()
{
    styleControlLabel(brandLabel, "Brand");
    styleControlLabel(deviceLabel, "Device");
    styleControlLabel(orientationLabel, "Orientation");
    styleComboBox(brandSelector);
    styleComboBox(deviceSelector);
    styleComboBox(orientationSelector);

    addAndMakeVisible(brandLabel);
    addAndMakeVisible(deviceLabel);
    addAndMakeVisible(orientationLabel);
    addAndMakeVisible(brandSelector);
    addAndMakeVisible(deviceSelector);
    addAndMakeVisible(orientationSelector);

    brandSelector.addItem("Apple", 1);
    brandSelector.addItem("Samsung", 2);
    brandSelector.setSelectedId(1, juce::dontSendNotification);
    orientationSelector.addItem("Portrait", 1);
    orientationSelector.addItem("Landscape", 2);
    orientationSelector.setSelectedId(1, juce::dontSendNotification);
    populateDevices("iphone-12-13-pro");

    brandSelector.onChange = [this]
    {
        populateDevices();
        updateTarget();
    };
    deviceSelector.onChange = [this]
    {
        updateTarget();
    };
    orientationSelector.onChange = [this]
    {
        updateTarget();
    };
    updateTarget();
}

void GuiLabComponent::populateDevices(const std::string &preferredDeviceId)
{
    visibleDevices.clear();
    deviceSelector.clear(juce::dontSendNotification);

    const auto selectedBrand =
        brandSelector.getSelectedId() == 2 ? "Samsung" : "Apple";
    int selectedId = 1;
    for (const auto &profile : getDeviceProfiles())
    {
        if (std::string(profile.brand) != selectedBrand)
        {
            continue;
        }

        visibleDevices.push_back(&profile);
        const auto itemId = static_cast<int>(visibleDevices.size());
        const auto label = juce::String(profile.name) + " (" +
                           juce::String(profile.portraitWidth) + " × " +
                           juce::String(profile.portraitHeight) + ")";
        deviceSelector.addItem(label, itemId);
        if (!preferredDeviceId.empty() && preferredDeviceId == profile.id)
        {
            selectedId = itemId;
        }
    }

    deviceSelector.setSelectedId(selectedId, juce::dontSendNotification);
}

void GuiLabComponent::updateTarget()
{
    const auto selectedIndex = deviceSelector.getSelectedId() - 1;
    if (workspace == nullptr || selectedIndex < 0 ||
        selectedIndex >= static_cast<int>(visibleDevices.size()))
    {
        return;
    }

    const auto orientation = orientationSelector.getSelectedId() == 2
                                 ? Orientation::landscape
                                 : Orientation::portrait;
    workspace->setTarget(*visibleDevices[static_cast<size_t>(selectedIndex)],
                         orientation);
}

void GuiLabComponent::layoutPalette()
{
    constexpr int gap = 12;
    const int availableWidth = std::max(1, paletteViewport.getWidth() - 16);
    constexpr float widestHardwareWidth = 230.f;
    const float scale =
        std::min(1.35f, static_cast<float>(availableWidth - gap * 2 - 24) /
                            widestHardwareWidth);

    int y = gap;

    for (auto &card : cards)
    {
        card->setHardwareScale(scale);
        const int width = card->requiredWidth();
        const int height = card->requiredHeight();
        card->setBounds(std::max(gap, (availableWidth - width) / 2), y, width,
                        height);
        y += height + gap;
    }

    paletteGallery.setSize(availableWidth, y);
}
