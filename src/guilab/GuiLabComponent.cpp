#include "GuiLabComponent.hpp"

#include "PreviewViewUtil.hpp"
#include "VmpcJuceResourceUtil.hpp"
#include "gui/vector/Constants.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <iterator>
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
         "main_screen_and_open_window", 90, 21},
        {"num-pad", "Num pad", "num_keys_lab", 85, 84},
        {"data-wheel", "DATA wheel", "data_wheel_unlabelled_lab", 72, 80},
        {"note-variation", "Note Variation + After/Assign",
         "note_variation_slider", 51, 130},
        {"tap-tempo", "Tap Tempo / Note Repeat", "tap_tempo_note_repeat", 51,
         30},
        {"undo-erase", "Undo Seq + Erase", "undo_seq_erase_lab", 58, 31},
        {"cursor", "Cursor", "cursor", 48, 48},
        {"cursor-compact", "Cursor - compact", "cursor_compact", 48, 31},
        {"locate", "Locate", "locate_group_lab", 179, 28},
        {"transport-horizontal", "Transport - horizontal", "transport_keys_lab",
         179, 30},
        {"transport-vertical", "Transport - vertical",
         "transport_keys_vertical", 33, 150},
        {"levels", "Full Level + 16 Levels", "full_level_16_levels", 69, 36},
        {"levels-compact", "Full Level + 16 Levels - compact",
         "full_level_16_levels_compact", 69, 23},
        {"sequence-mute", "Next Seq + Track Mute", "next_seq_track_mute",
         69, 23},
        {"pads-banks", "Pads + Pad Bank", "pads_with_banks_lab", 180, 224},
        {"pads", "Pads - compact", "pads_lab", 180, 188},
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

    struct PixelSpan
    {
        int start;
        int end;
    };

    PixelSpan projectPixelSpan(const float logicalStart,
                               const float logicalExtent, const float zoom,
                               const int limit)
    {
        auto start = std::clamp(
            juce::roundToInt(logicalStart * zoom), 0, limit);
        auto end = std::clamp(
            juce::roundToInt((logicalStart + logicalExtent) * zoom), 0,
            limit);

        // Keep very small projected nodes visible without allowing their
        // component bounds to extend beyond the document surface.
        if (end <= start && limit > 0)
        {
            start = std::min(start, limit - 1);
            end = start + 1;
        }

        return {start, end};
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
private:
    class ArrangementNode;

public:
    explicit ArrangementWorkspace(const DeviceProfile &deviceToUse)
        : device(deviceToUse)
    {
        document.orientation = Orientation::portrait;
        document.referenceDeviceId = device.id;
        document.referenceSize =
            getEffectiveDeviceSize(device, document.orientation);
        setWantsKeyboardFocus(true);
        setFocusContainerType(
            juce::Component::FocusContainerType::focusContainer);

        groupButton.onClick = [this]
        {
            groupSelection();
        };
        ungroupButton.onClick = [this]
        {
            ungroupSelection();
        };
        for (auto *button : {&groupButton, &ungroupButton})
        {
            button->setColour(juce::TextButton::buttonColourId,
                              juce::Colour(0xff343a38));
            button->setColour(juce::TextButton::textColourOffId,
                              juce::Colour(0xffedf2ef));
            addAndMakeVisible(*button);
        }

        const std::array<juce::juce_wchar, 9> labelCharacters{
            0x2196, 0x2191, 0x2197, 0x2190, 0x2022,
            0x2192, 0x2199, 0x2193, 0x2198};
        for (size_t i = 0; i < anchorButtons.size(); ++i)
        {
            auto button = std::make_unique<juce::TextButton>(
                juce::String::charToString(labelCharacters[i]));
            button->setTooltip("Anchor selected component");
            button->onClick = [this, i]
            {
                setSelectedAnchor(
                    {static_cast<AnchorAxis>(i % 3),
                     static_cast<AnchorAxis>(i / 3)});
            };
            addAndMakeVisible(*button);
            anchorButtons[i] = std::move(button);
        }
        refreshSelection();
        refreshResponsiveLayout();
    }

    void setTarget(const DeviceProfile &newDevice,
                   const Orientation newOrientation)
    {
        device = newDevice;
        orientation = newOrientation;

        // Before the first component is added, the selected screen establishes
        // the document's canonical coordinate space. Afterwards target changes
        // are previews only and never rewrite authored geometry.
        if (document.nodes.empty())
        {
            document.orientation = orientation;
            document.referenceDeviceId = device.id;
            document.referenceSize =
                getEffectiveDeviceSize(device, orientation);
        }

        refreshResponsiveLayout();
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
            juce::String(juce::roundToInt(zoom * 100.f)) + "% view — " +
            juce::String(
                juce::roundToInt(responsiveLayout.sharedScale * 100.f)) +
            "% responsive scale";
        g.setColour(juce::Colour(0xffaeb7b3));
        g.setFont(13.f);
        g.drawText(description, getLocalBounds().removeFromTop(54),
                   juce::Justification::centred);

        g.setColour(Constants::chassisColour);
        g.fillRect(surfaceBounds);
        const auto borderWidth = isDragOverSurface ? 3 : 2;
        g.setColour(isDragOverSurface ? juce::Colour(0xff43b3dd)
                                      : juce::Colour(0xff87918e));
        g.drawRect(surfaceBounds.expanded(borderWidth), borderWidth);
    }

    void resized() override
    {
        auto available = getLocalBounds().reduced(18);
        auto toolbar = available.removeFromTop(54);

        groupButton.setBounds(toolbar.removeFromLeft(66).withHeight(24));
        toolbar.removeFromLeft(6);
        ungroupButton.setBounds(toolbar.removeFromLeft(76).withHeight(24));
        constexpr int anchorButtonSize = 17;
        auto anchorArea = getLocalBounds().reduced(18).removeFromTop(51);
        anchorArea = anchorArea.removeFromRight(anchorButtonSize * 3);
        for (size_t i = 0; i < anchorButtons.size(); ++i)
        {
            anchorButtons[i]->setBounds(
                anchorArea.getX() + static_cast<int>(i % 3) * anchorButtonSize,
                anchorArea.getY() + static_cast<int>(i / 3) * anchorButtonSize,
                anchorButtonSize, anchorButtonSize);
        }

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

        refreshResponsiveLayout();
        for (auto &node : nodeComponents)
        {
            updateNodeBounds(*node);
        }
        bringToolbarToFront();
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (event.eventComponent == this)
        {
            clearSelection();
        }
        grabKeyboardFocus();
    }

    bool keyPressed(const juce::KeyPress &key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            clearSelection();
            return true;
        }

        const auto commandDown = key.getModifiers().isCommandDown();
        if (commandDown && (key.getKeyCode() == 'G' ||
                            key.getKeyCode() == 'g'))
        {
            if (key.getModifiers().isShiftDown())
            {
                ungroupSelection();
            }
            else
            {
                groupSelection();
            }
            return true;
        }

        if (!selectedIds.empty() && (key == juce::KeyPress::deleteKey ||
                                     key == juce::KeyPress::backspaceKey))
        {
            document.nodes.erase(
                std::remove_if(document.nodes.begin(), document.nodes.end(),
                               [this](const auto &node)
                               {
                                   return isSelected(node.id);
                               }),
                document.nodes.end());
            selectedIds.clear();
            rebuildNodeComponents();
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
        ArrangementNodeModel model;
        model.id = nextNodeId++;
        model.catalogId = entry->id;
        model.referenceSize = referenceSize;
        const auto projectedScale =
            constrainItemScale(1.f, referenceSize, deviceSize);
        model.scale = projectedScale /
                      std::max(0.0001f, responsiveLayout.sharedScale);

        const auto logicalDropX =
            static_cast<float>(details.localPosition.x - surfaceBounds.getX()) /
            zoom;
        const auto logicalDropY =
            static_cast<float>(details.localPosition.y - surfaceBounds.getY()) /
            zoom;
        const LogicalSize itemSize{referenceSize.width * projectedScale,
                                   referenceSize.height * projectedScale};
        const LogicalPoint requested{logicalDropX - itemSize.width * 0.5f,
                                     logicalDropY - itemSize.height * 0.5f};
        const auto shouldSnap =
            !juce::ModifierKeys::getCurrentModifiersRealtime().isAltDown();
        const auto projectedPosition =
            constrainItemPosition(requested, itemSize, deviceSize, shouldSnap);
        model.anchor = inferAnchor(projectedPosition, itemSize, deviceSize);
        model.position = unprojectNodePosition(
            model, projectedPosition, responsiveLayout.transform,
            responsiveLayout.sharedScale);
        const auto nearest = findNearestValidPosition(
            document, model, model.position, shouldSnap);
        if (!nearest.has_value())
        {
            repaint();
            return;
        }
        model.position = *nearest;

        document.nodes.push_back(std::move(model));
        selectedIds = {document.nodes.back().id};
        refreshResponsiveLayout();
        rebuildNodeComponents();
        grabKeyboardFocus();
        repaint();
    }

private:
    class ArrangementNode final : public juce::Component
    {
    public:
        class InteractionLayer final : public juce::Component
        {
        public:
            explicit InteractionLayer(ArrangementNode &ownerToUse)
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

            void mouseUp(const juce::MouseEvent &) override
            {
                owner.workspace.endGesture();
            }

        private:
            ArrangementNode &owner;
        };

        class ResizeHandle final : public juce::Component
        {
        public:
            ResizeHandle(ArrangementNode &ownerToUse,
                         const ResizeCorner cornerToUse)
                : owner(ownerToUse), corner(cornerToUse)
            {
                auto cursor = juce::MouseCursor::BottomRightCornerResizeCursor;
                switch (corner)
                {
                case ResizeCorner::topLeft:
                    cursor = juce::MouseCursor::TopLeftCornerResizeCursor;
                    break;
                case ResizeCorner::topRight:
                    cursor = juce::MouseCursor::TopRightCornerResizeCursor;
                    break;
                case ResizeCorner::bottomLeft:
                    cursor = juce::MouseCursor::BottomLeftCornerResizeCursor;
                    break;
                case ResizeCorner::bottomRight:
                    break;
                }
                setMouseCursor(cursor);
            }

            void paint(juce::Graphics &g) override
            {
                g.setColour(juce::Colour(0xff43b3dd));
                g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.f),
                                       2.f);
                g.setColour(juce::Colour(0xffedf2ef));
                const auto descending =
                    corner == ResizeCorner::topLeft ||
                    corner == ResizeCorner::bottomRight;
                g.drawLine(4.f, descending ? 4.f
                                           : static_cast<float>(getHeight() - 4),
                           static_cast<float>(getWidth() - 4),
                           descending ? static_cast<float>(getHeight() - 4)
                                      : 4.f,
                           1.5f);
            }

            void mouseDown(const juce::MouseEvent &event) override
            {
                owner.beginResize(corner, event);
            }

            void mouseDrag(const juce::MouseEvent &event) override
            {
                owner.continueResize(corner, event);
            }

            void mouseUp(const juce::MouseEvent &) override
            {
                owner.workspace.endGesture();
            }

        private:
            ArrangementNode &owner;
            ResizeCorner corner;
        };

        ArrangementNode(ArrangementWorkspace &workspaceToUse,
                        const std::uint64_t nodeIdToUse)
            : workspace(workspaceToUse), nodeId(nodeIdToUse),
              interactionLayer(*this)
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            const auto *model = workspace.findNode(nodeId);
            if (model != nullptr && model->isGroup())
            {
                for (const auto &child : model->children)
                {
                    if (const auto *entry = findCatalogEntry(child.catalogId))
                    {
                        auto preview = std::make_unique<PreviewComponent>(*entry);
                        preview->setInterceptsMouseClicks(false, false);
                        addAndMakeVisible(*preview);
                        previews.push_back(std::move(preview));
                    }
                }
            }
            else if (model != nullptr)
            {
                if (const auto *entry = findCatalogEntry(model->catalogId))
                {
                    auto preview = std::make_unique<PreviewComponent>(*entry);
                    preview->setInterceptsMouseClicks(false, false);
                    addAndMakeVisible(*preview);
                    previews.push_back(std::move(preview));
                }
            }
            addAndMakeVisible(interactionLayer);
            for (const auto corner :
                 {ResizeCorner::topLeft, ResizeCorner::topRight,
                  ResizeCorner::bottomLeft, ResizeCorner::bottomRight})
            {
                auto handle = std::make_unique<ResizeHandle>(*this, corner);
                handle->setVisible(false);
                addAndMakeVisible(*handle);
                resizeHandles.push_back(std::move(handle));
            }
        }

        LogicalSize getReferenceSize() const
        {
            if (const auto *model = workspace.findNode(nodeId))
            {
                return model->referenceSize;
            }
            return {};
        }

        std::uint64_t getNodeId() const
        {
            return nodeId;
        }

        void setSelected(const bool shouldBeSelected,
                         const bool showResizeHandle)
        {
            selected = shouldBeSelected;
            for (auto &handle : resizeHandles)
            {
                handle->setVisible(selected && showResizeHandle);
            }
            interactionLayer.repaint();
            repaint();
        }

        void resized() override
        {
            layoutPreviews();
            interactionLayer.setBounds(getLocalBounds());
            constexpr int handleSize = 16;
            const std::array<juce::Point<int>, 4> positions{{
                {0, 0},
                {getWidth() - handleSize, 0},
                {0, getHeight() - handleSize},
                {getWidth() - handleSize, getHeight() - handleSize}}};
            for (size_t i = 0; i < resizeHandles.size(); ++i)
            {
                resizeHandles[i]->setBounds(positions[i].x, positions[i].y,
                                             handleSize, handleSize);
                resizeHandles[i]->toFront(false);
            }
        }

    private:
        void beginMove(const juce::MouseEvent &event)
        {
            workspace.selectNode(nodeId, event.mods.isShiftDown());
            workspace.grabKeyboardFocus();
            workspace.beginGesture();
            if (const auto *model = workspace.findNode(nodeId))
            {
                startPosition = model->position;
            }
            startScreenPosition = event.getScreenPosition();
        }

        void continueMove(const juce::MouseEvent &event)
        {
            const auto delta = event.getScreenPosition() - startScreenPosition;
            const auto positionScale = std::max(
                0.0001f, workspace.responsiveLayout.transform.scale);
            const LogicalPoint requested{
                startPosition.x + static_cast<float>(delta.x) /
                                      (workspace.zoom * positionScale),
                startPosition.y + static_cast<float>(delta.y) /
                                      (workspace.zoom * positionScale)};
            workspace.moveNode(nodeId, requested, !event.mods.isAltDown());
        }

        void beginResize(const ResizeCorner corner,
                         const juce::MouseEvent &event)
        {
            workspace.selectNode(nodeId, false);
            workspace.grabKeyboardFocus();
            workspace.beginGesture();
            resizeStartGeometry = workspace.getProjectedGeometry(nodeId);
            resizeStartScreenPosition = event.getScreenPosition();
            resizeCorner = corner;
        }

        void continueResize(const ResizeCorner corner,
                            const juce::MouseEvent &event)
        {
            if (corner != resizeCorner)
            {
                return;
            }
            workspace.resizeNode(
                nodeId, corner, resizeStartGeometry,
                event.getScreenPosition() - resizeStartScreenPosition,
                event.mods.isShiftDown(), !event.mods.isAltDown());
        }

        void layoutPreviews()
        {
            const auto *model = workspace.findNode(nodeId);
            if (model == nullptr || previews.empty())
            {
                return;
            }

            if (!model->isGroup())
            {
                previews.front()->setHardwareScale(
                    model->scale * workspace.responsiveLayout.sharedScale *
                    workspace.zoom);
                previews.front()->setBounds(getLocalBounds());
                return;
            }

            size_t previewIndex = 0;
            for (const auto &child : model->children)
            {
                if (findCatalogEntry(child.catalogId) == nullptr ||
                    previewIndex >= previews.size())
                {
                    continue;
                }
                const auto localScale =
                    model->scale * workspace.responsiveLayout.sharedScale *
                    workspace.zoom;
                auto &preview = *previews[previewIndex++];
                preview.setHardwareScale(child.scale * localScale);
                preview.setBounds(
                    juce::roundToInt(child.position.x * localScale),
                    juce::roundToInt(child.position.y * localScale),
                    std::max(1, juce::roundToInt(
                                    child.referenceSize.width * child.scale *
                                    localScale)),
                    std::max(1, juce::roundToInt(
                                    child.referenceSize.height * child.scale *
                                    localScale)));
            }
        }

        ArrangementWorkspace &workspace;
        std::uint64_t nodeId;
        std::vector<std::unique_ptr<PreviewComponent>> previews;
        InteractionLayer interactionLayer;
        std::vector<std::unique_ptr<ResizeHandle>> resizeHandles;
        LogicalPoint startPosition;
        juce::Point<int> startScreenPosition;
        ProjectedNodeGeometry resizeStartGeometry;
        juce::Point<int> resizeStartScreenPosition;
        ResizeCorner resizeCorner = ResizeCorner::bottomRight;
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

    ArrangementNodeModel *findNode(const std::uint64_t id)
    {
        const auto found = std::find_if(document.nodes.begin(),
                                        document.nodes.end(),
                                        [id](const auto &node)
                                        {
                                            return node.id == id;
                                        });
        return found == document.nodes.end() ? nullptr : &*found;
    }

    const ArrangementNodeModel *findNode(const std::uint64_t id) const
    {
        const auto found = std::find_if(document.nodes.begin(),
                                        document.nodes.end(),
                                        [id](const auto &node)
                                        {
                                            return node.id == id;
                                        });
        return found == document.nodes.end() ? nullptr : &*found;
    }

    void refreshResponsiveLayout()
    {
        const auto targetSize = getEffectiveDeviceSize(device, orientation);
        if (!frozenResponsiveScale.has_value() ||
            !frozenResponsiveLayout.has_value())
        {
            responsiveLayout =
                computeResponsiveLayout(document, targetSize);
            return;
        }

        responsiveLayout = projectDocumentAtScale(
            document, targetSize, *frozenResponsiveScale);
        for (auto &projected : responsiveLayout.nodes)
        {
            const auto *frozen = findProjectedGeometry(
                *frozenResponsiveLayout, projected.id);
            if (frozen == nullptr)
            {
                continue;
            }
            projected.geometry.reflowOffset = frozen->reflowOffset;
            projected.geometry.position.x += frozen->reflowOffset.x;
            projected.geometry.position.y += frozen->reflowOffset.y;
        }
    }

    void beginGesture()
    {
        if (!frozenResponsiveScale.has_value())
        {
            frozenResponsiveScale = responsiveLayout.sharedScale;
            frozenResponsiveLayout = responsiveLayout;
        }
    }

    void endGesture()
    {
        if (!frozenResponsiveScale.has_value())
        {
            return;
        }
        frozenResponsiveScale.reset();
        frozenResponsiveLayout.reset();
        refreshResponsiveLayout();
        for (auto &component : nodeComponents)
        {
            updateNodeBounds(*component);
        }
        repaint();
    }

    ProjectedNodeGeometry getProjectedGeometry(const std::uint64_t id) const
    {
        if (const auto *geometry =
                findProjectedGeometry(responsiveLayout, id))
        {
            return *geometry;
        }
        return {};
    }

    bool isSelected(const std::uint64_t id) const
    {
        return std::find(selectedIds.begin(), selectedIds.end(), id) !=
               selectedIds.end();
    }

    void clearSelection()
    {
        selectedIds.clear();
        refreshSelection();
    }

    void selectNode(const std::uint64_t id, const bool additive)
    {
        if (additive)
        {
            const auto found = std::find(selectedIds.begin(), selectedIds.end(),
                                         id);
            if (found == selectedIds.end())
            {
                selectedIds.push_back(id);
            }
            else
            {
                selectedIds.erase(found);
            }
        }
        else if (!isSelected(id) || selectedIds.size() == 1)
        {
            selectedIds = {id};
        }
        refreshSelection();
    }

    void moveNode(const std::uint64_t id,
                  const LogicalPoint requestedPosition,
                  const bool shouldSnap)
    {
        auto *node = findNode(id);
        if (node == nullptr)
        {
            return;
        }
        auto candidate = *node;
        candidate.position = requestedPosition;
        const auto nearest = findNearestValidPosition(
            document, candidate, candidate.position, shouldSnap, id);
        if (!nearest.has_value())
        {
            return;
        }
        node->position = *nearest;
        refreshResponsiveLayout();
        updateAllNodeBounds();
    }

    void resizeNode(const std::uint64_t id,
                    const ResizeCorner corner,
                    const ProjectedNodeGeometry startGeometry,
                    const juce::Point<int> screenDelta,
                    const bool useCentrePivot, const bool shouldSnap)
    {
        auto *node = findNode(id);
        if (node == nullptr)
        {
            return;
        }
        const auto referenceSize = node->referenceSize;
        const auto horizontalDirection =
            corner == ResizeCorner::topLeft ||
                    corner == ResizeCorner::bottomLeft
                ? -1.f
                : 1.f;
        const auto verticalDirection =
            corner == ResizeCorner::topLeft ||
                    corner == ResizeCorner::topRight
                ? -1.f
                : 1.f;
        const auto denominator =
            zoom * (referenceSize.width * referenceSize.width +
                    referenceSize.height * referenceSize.height);
        if (denominator <= 0.f)
        {
            return;
        }

        auto requestedScale =
            startGeometry.scale +
            (static_cast<float>(screenDelta.x) * horizontalDirection *
                 referenceSize.width +
             static_cast<float>(screenDelta.y) * verticalDirection *
                 referenceSize.height) /
                denominator;
        if (shouldSnap)
        {
            requestedScale = snapItemScaleToGrid(requestedScale, referenceSize);
        }
        const auto sharedScale = std::max(
            0.0001f,
            frozenResponsiveScale.value_or(responsiveLayout.sharedScale));
        requestedScale = std::clamp(requestedScale, 0.5f * sharedScale,
                                    6.f * sharedScale);

        const auto makeCandidate = [&](const float projectedScale)
        {
            auto candidate = *node;
            candidate.scale = projectedScale / sharedScale;
            const LogicalSize requestedProjectedSize{
                referenceSize.width * projectedScale,
                referenceSize.height * projectedScale};
            const auto displayedPosition = positionForResizedNode(
                startGeometry, requestedProjectedSize, corner,
                useCentrePivot);
            const LogicalPoint idealPosition{
                displayedPosition.x - startGeometry.reflowOffset.x,
                displayedPosition.y - startGeometry.reflowOffset.y};
            candidate.position = unprojectNodePosition(
                candidate, idealPosition, responsiveLayout.transform,
                sharedScale);
            return candidate;
        };

        auto candidate = makeCandidate(requestedScale);
        if (!isNodePlacementValid(document, candidate, id))
        {
            if (requestedScale <= startGeometry.scale)
            {
                return;
            }
            auto lower = startGeometry.scale;
            auto upper = requestedScale;
            for (int iteration = 0; iteration < 32; ++iteration)
            {
                const auto scale = (lower + upper) * 0.5f;
                if (isNodePlacementValid(document, makeCandidate(scale), id))
                {
                    lower = scale;
                }
                else
                {
                    upper = scale;
                }
            }
            candidate = makeCandidate(lower);
        }

        *node = std::move(candidate);
        refreshResponsiveLayout();
        updateAllNodeBounds();
    }

    void updateNodeBounds(ArrangementNode &component)
    {
        const auto geometry = getProjectedGeometry(component.getNodeId());

        // Round both edges of the projected rectangle instead of rounding its
        // position and size independently. The latter can make the right or
        // bottom edge one pixel too large and let a component paint over the
        // document's exterior border.
        const auto horizontal = projectPixelSpan(
            geometry.position.x, geometry.size.width, zoom,
            surfaceBounds.getWidth());
        const auto vertical = projectPixelSpan(
            geometry.position.y, geometry.size.height, zoom,
            surfaceBounds.getHeight());
        component.setBounds(
            surfaceBounds.getX() + horizontal.start,
            surfaceBounds.getY() + vertical.start,
            horizontal.end - horizontal.start, vertical.end - vertical.start);
        component.resized();
    }

    void updateNodeBoundsById(const std::uint64_t id)
    {
        const auto found = std::find_if(nodeComponents.begin(),
                                        nodeComponents.end(),
                                        [id](const auto &component)
                                        {
                                            return component->getNodeId() == id;
                                        });
        if (found != nodeComponents.end())
        {
            updateNodeBounds(**found);
        }
        refreshSelection();
    }

    void updateAllNodeBounds()
    {
        for (auto &component : nodeComponents)
        {
            updateNodeBounds(*component);
        }
        refreshSelection();
    }

    void rebuildNodeComponents()
    {
        refreshResponsiveLayout();
        nodeComponents.clear();
        for (const auto &node : document.nodes)
        {
            auto component = std::make_unique<ArrangementNode>(*this, node.id);
            addAndMakeVisible(*component);
            updateNodeBounds(*component);
            nodeComponents.push_back(std::move(component));
        }
        refreshSelection();
        repaint();
    }

    void refreshSelection()
    {
        const auto single = selectedIds.size() == 1;
        for (auto &component : nodeComponents)
        {
            component->setSelected(isSelected(component->getNodeId()), single);
        }

        auto canGroup = selectedIds.size() >= 2 &&
                        std::all_of(selectedIds.begin(), selectedIds.end(),
                                    [this](const auto id)
                                    {
                                        const auto *node = findNode(id);
                                        return node != nullptr &&
                                               !node->isGroup();
                                    });
        if (canGroup)
        {
            std::vector<ArrangementNodeModel> selectedNodes;
            for (const auto &node : document.nodes)
            {
                if (isSelected(node.id))
                {
                    selectedNodes.push_back(node);
                }
            }
            const auto prospective =
                makeFixedGroup(0, selectedNodes, document.referenceSize);
            const auto prospectiveRect = getNodeRect(prospective);
            canGroup = std::none_of(
                document.nodes.begin(), document.nodes.end(),
                [this, prospectiveRect](const auto &node)
                {
                    return !isSelected(node.id) &&
                           rectanglesOverlap(prospectiveRect,
                                             getNodeRect(node));
                });
        }
        const auto *singleNode =
            single ? findNode(selectedIds.front()) : nullptr;
        groupButton.setEnabled(canGroup);
        ungroupButton.setEnabled(singleNode != nullptr && singleNode->isGroup());

        for (size_t i = 0; i < anchorButtons.size(); ++i)
        {
            auto &button = *anchorButtons[i];
            button.setEnabled(singleNode != nullptr);
            const ArrangementAnchor represented{
                static_cast<AnchorAxis>(i % 3),
                static_cast<AnchorAxis>(i / 3)};
            button.setColour(
                juce::TextButton::buttonColourId,
                singleNode != nullptr && singleNode->anchor == represented
                    ? juce::Colour(0xff43b3dd)
                    : juce::Colour(0xff343a38));
        }
        bringToolbarToFront();
    }

    void bringToolbarToFront()
    {
        groupButton.toFront(false);
        ungroupButton.toFront(false);
        for (auto &button : anchorButtons)
        {
            if (button != nullptr)
            {
                button->toFront(false);
            }
        }
    }

    void setSelectedAnchor(const ArrangementAnchor anchor)
    {
        if (selectedIds.size() != 1)
        {
            return;
        }
        auto *node = findNode(selectedIds.front());
        if (node == nullptr || node->anchor == anchor)
        {
            return;
        }
        const auto original = *node;
        const auto projected = getProjectedGeometry(node->id);
        const LogicalPoint idealPosition{
            projected.position.x - projected.reflowOffset.x,
            projected.position.y - projected.reflowOffset.y};
        node->anchor = anchor;
        node->position = unprojectNodePosition(
            *node, idealPosition, responsiveLayout.transform,
            responsiveLayout.sharedScale);
        const auto nearest = findNearestValidPosition(
            document, *node, node->position, false, node->id);
        if (!nearest.has_value())
        {
            *node = original;
            return;
        }
        node->position = *nearest;
        refreshResponsiveLayout();
        updateAllNodeBounds();
    }

    void groupSelection()
    {
        std::vector<ArrangementNodeModel> selectedNodes;
        size_t insertionIndex = document.nodes.size();
        for (size_t i = 0; i < document.nodes.size(); ++i)
        {
            if (isSelected(document.nodes[i].id))
            {
                if (document.nodes[i].isGroup())
                {
                    return;
                }
                insertionIndex = std::min(insertionIndex, i);
                selectedNodes.push_back(document.nodes[i]);
            }
        }
        if (selectedNodes.size() < 2)
        {
            return;
        }

        auto group = makeFixedGroup(nextNodeId++, selectedNodes,
                                    document.referenceSize);
        const auto groupRect = getNodeRect(group);
        if (std::any_of(document.nodes.begin(), document.nodes.end(),
                        [this, groupRect](const auto &node)
                        {
                            return !isSelected(node.id) &&
                                   rectanglesOverlap(groupRect,
                                                     getNodeRect(node));
                        }))
        {
            return;
        }
        document.nodes.erase(
            std::remove_if(document.nodes.begin(), document.nodes.end(),
                           [this](const auto &node)
                           {
                               return isSelected(node.id);
                           }),
            document.nodes.end());
        insertionIndex = std::min(insertionIndex, document.nodes.size());
        const auto groupId = group.id;
        document.nodes.insert(document.nodes.begin() +
                                  static_cast<std::ptrdiff_t>(insertionIndex),
                              std::move(group));
        selectedIds = {groupId};
        refreshResponsiveLayout();
        rebuildNodeComponents();
    }

    void ungroupSelection()
    {
        if (selectedIds.size() != 1)
        {
            return;
        }
        const auto groupId = selectedIds.front();
        const auto found = std::find_if(document.nodes.begin(),
                                        document.nodes.end(),
                                        [groupId](const auto &node)
                                        {
                                            return node.id == groupId;
                                        });
        if (found == document.nodes.end() || !found->isGroup())
        {
            return;
        }
        const auto index = static_cast<size_t>(
            std::distance(document.nodes.begin(), found));
        auto children = ungroupFixedGroup(*found, document.referenceSize);
        document.nodes.erase(found);
        selectedIds.clear();
        for (const auto &child : children)
        {
            selectedIds.push_back(child.id);
        }
        document.nodes.insert(
            document.nodes.begin() + static_cast<std::ptrdiff_t>(index),
            std::make_move_iterator(children.begin()),
            std::make_move_iterator(children.end()));
        refreshResponsiveLayout();
        rebuildNodeComponents();
    }

    DeviceProfile device;
    Orientation orientation = Orientation::portrait;
    ArrangementDocument document;
    ResponsiveLayout responsiveLayout;
    std::optional<float> frozenResponsiveScale;
    std::optional<ResponsiveLayout> frozenResponsiveLayout;
    std::uint64_t nextNodeId = 1;
    float zoom = 1.f;
    juce::Rectangle<int> surfaceBounds;
    std::vector<std::unique_ptr<ArrangementNode>> nodeComponents;
    std::vector<std::uint64_t> selectedIds;
    juce::TextButton groupButton{"Group"};
    juce::TextButton ungroupButton{"Ungroup"};
    std::array<std::unique_ptr<juce::TextButton>, 9> anchorButtons;
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
