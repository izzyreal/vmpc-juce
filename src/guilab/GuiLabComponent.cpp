#include "GuiLabComponent.hpp"

#include "gui/arrangement/PreviewViewUtil.hpp"
#include "VmpcJuceResourceUtil.hpp"
#include "gui/vector/Constants.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <iterator>
#include <limits>
#include <string>

using namespace vmpc_juce::guilab;
using namespace vmpc_juce::gui::arrangement;
using vmpc_juce::gui::vector::Constants;

namespace
{
    constexpr auto dragDescriptionPrefix = "component:";
    constexpr float arrangementGridSize = 4.f;
    constexpr float fineKeyboardMove = 1.f;
    constexpr float largeKeyboardMove = arrangementGridSize * 4.f;
    constexpr auto recentDesignFileKey = "recentDesignFile";
    constexpr auto recentSetupFileKey = "recentSetupFile";
    constexpr auto recentDocumentKindKey = "recentDocumentKind";
    constexpr auto designDocumentKind = "design";
    constexpr auto setupDocumentKind = "setup";

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
        auto start =
            std::clamp(juce::roundToInt(logicalStart * zoom), 0, limit);
        auto end = std::clamp(
            juce::roundToInt((logicalStart + logicalExtent) * zoom), 0, limit);

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
        setWantsKeyboardFocus(true);
        setFocusContainerType(
            juce::Component::FocusContainerType::focusContainer);

        refreshSelection();
        refreshResponsiveLayout();
    }

    void setTarget(const DeviceProfile &newDevice,
                   const Orientation newOrientation)
    {
        device = newDevice;
        orientation = newOrientation;

        refreshResponsiveLayout();
        resized();
        repaint();
    }

    ArrangementDocument getDocument() const
    {
        return document;
    }

    bool loadDocument(const ArrangementDocument &newDocument,
                      std::string &errorMessage)
    {
        for (const auto &node : newDocument.nodes)
        {
            if (findCatalogEntry(node.catalogId) == nullptr)
            {
                errorMessage =
                    "The design uses an unknown component: " + node.catalogId;
                return false;
            }
        }

        document = newDocument;
        frozenResponsiveScale.reset();
        selectedIds.clear();
        nextNodeId = 1;
        for (const auto &node : document.nodes)
        {
            nextNodeId = std::max(nextNodeId, node.id + 1);
        }
        rebuildNodeComponents();
        resized();
        repaint();
        errorMessage.clear();
        return true;
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
        available.removeFromTop(54);
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

        const auto modifiers = key.getModifiers();
        const auto commandDown = modifiers.isCommandDown();
        const auto selectAllShortcutDown =
            commandDown || modifiers.isCtrlDown();
        if (selectAllShortcutDown &&
            (key.getKeyCode() == 'A' || key.getKeyCode() == 'a'))
        {
            selectAllNodes();
            return true;
        }

        LogicalPoint keyboardDirection;
        if (key.isKeyCode(juce::KeyPress::leftKey))
        {
            keyboardDirection.x = -1.f;
        }
        else if (key.isKeyCode(juce::KeyPress::rightKey))
        {
            keyboardDirection.x = 1.f;
        }
        else if (key.isKeyCode(juce::KeyPress::upKey))
        {
            keyboardDirection.y = -1.f;
        }
        else if (key.isKeyCode(juce::KeyPress::downKey))
        {
            keyboardDirection.y = 1.f;
        }

        if (!selectAllShortcutDown && !selectedIds.empty() &&
            (keyboardDirection.x != 0.f || keyboardDirection.y != 0.f))
        {
            const auto fineMove = modifiers.isAltDown();
            const auto distance = fineMove ? fineKeyboardMove
                                  : modifiers.isShiftDown()
                                      ? largeKeyboardMove
                                      : arrangementGridSize;
            moveSelection({keyboardDirection.x * distance,
                           keyboardDirection.y * distance},
                          !fineMove);
            return true;
        }

        if (!selectedIds.empty() && (key == juce::KeyPress::deleteKey ||
                                     key == juce::KeyPress::backspaceKey))
        {
            document.nodes.erase(std::remove_if(document.nodes.begin(),
                                                document.nodes.end(),
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
        model.widthFraction =
            referenceSize.width * projectedScale / deviceSize.width;

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
        auto projectedPosition =
            constrainItemPosition(requested, itemSize, deviceSize, shouldSnap);
        std::vector<LogicalRect> obstacles;
        obstacles.reserve(responsiveLayout.nodes.size());
        for (const auto &projected : responsiveLayout.nodes)
        {
            obstacles.push_back(
                {projected.geometry.position, projected.geometry.size});
        }
        if (const auto nearest = findNearestAvailablePosition(
                projectedPosition, itemSize, deviceSize, obstacles, shouldSnap))
        {
            projectedPosition = *nearest;
        }
        model.center = normalizedCenter(
            {projectedPosition, itemSize, projectedScale}, deviceSize);

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
                        cursor =
                            juce::MouseCursor::BottomLeftCornerResizeCursor;
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
                const auto descending = corner == ResizeCorner::topLeft ||
                                        corner == ResizeCorner::bottomRight;
                g.drawLine(
                    4.f, descending ? 4.f : static_cast<float>(getHeight() - 4),
                    static_cast<float>(getWidth() - 4),
                    descending ? static_cast<float>(getHeight() - 4) : 4.f,
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
            if (model != nullptr)
            {
                if (const auto *entry = findCatalogEntry(model->catalogId))
                {
                    preview = std::make_unique<PreviewComponent>(*entry);
                    preview->setInterceptsMouseClicks(false, false);
                    addAndMakeVisible(*preview);
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
            const std::array<juce::Point<int>, 4> positions{
                {{0, 0},
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
            startPosition = workspace.getProjectedGeometry(nodeId).position;
            startScreenPosition = event.getScreenPosition();
        }

        void continueMove(const juce::MouseEvent &event)
        {
            const auto delta = event.getScreenPosition() - startScreenPosition;
            if (delta.isOrigin())
            {
                return;
            }
            const LogicalPoint requested{
                startPosition.x + static_cast<float>(delta.x) / workspace.zoom,
                startPosition.y + static_cast<float>(delta.y) / workspace.zoom};
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
            const auto delta =
                event.getScreenPosition() - resizeStartScreenPosition;
            if (delta.isOrigin())
            {
                return;
            }
            workspace.resizeNode(nodeId, corner, resizeStartGeometry, delta,
                                 event.mods.isShiftDown(),
                                 !event.mods.isAltDown());
        }

        void layoutPreviews()
        {
            const auto *model = workspace.findNode(nodeId);
            if (model == nullptr || preview == nullptr)
            {
                return;
            }

            const auto geometry = workspace.getProjectedGeometry(nodeId);
            preview->setHardwareScale(geometry.scale * workspace.zoom);
            preview->setBounds(getLocalBounds());
        }

        ArrangementWorkspace &workspace;
        std::uint64_t nodeId;
        std::unique_ptr<PreviewComponent> preview;
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
        const auto found =
            std::find_if(document.nodes.begin(), document.nodes.end(),
                         [id](const auto &node)
                         {
                             return node.id == id;
                         });
        return found == document.nodes.end() ? nullptr : &*found;
    }

    const ArrangementNodeModel *findNode(const std::uint64_t id) const
    {
        const auto found =
            std::find_if(document.nodes.begin(), document.nodes.end(),
                         [id](const auto &node)
                         {
                             return node.id == id;
                         });
        return found == document.nodes.end() ? nullptr : &*found;
    }

    void refreshResponsiveLayout()
    {
        const auto targetSize = getEffectiveDeviceSize(device, orientation);
        if (!frozenResponsiveScale.has_value())
        {
            responsiveLayout = computeResponsiveLayout(document, targetSize);
            return;
        }

        responsiveLayout = projectDocumentAtScale(document, targetSize,
                                                  *frozenResponsiveScale);
    }

    void beginGesture()
    {
        if (!frozenResponsiveScale.has_value())
        {
            frozenResponsiveScale = responsiveLayout.sharedScale;
        }
    }

    void endGesture()
    {
        if (!frozenResponsiveScale.has_value())
        {
            return;
        }
        frozenResponsiveScale.reset();
        refreshResponsiveLayout();
        for (auto &component : nodeComponents)
        {
            updateNodeBounds(*component);
        }
        repaint();
    }

    ProjectedNodeGeometry getProjectedGeometry(const std::uint64_t id) const
    {
        if (const auto *geometry = findProjectedGeometry(responsiveLayout, id))
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

    void selectAllNodes()
    {
        selectedIds.clear();
        selectedIds.reserve(document.nodes.size());
        for (const auto &node : document.nodes)
        {
            selectedIds.push_back(node.id);
        }
        refreshSelection();
    }

    void selectNode(const std::uint64_t id, const bool additive)
    {
        if (additive)
        {
            const auto found =
                std::find(selectedIds.begin(), selectedIds.end(), id);
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

    void moveNode(const std::uint64_t id, const LogicalPoint requestedPosition,
                  const bool shouldSnap)
    {
        auto *node = findNode(id);
        if (node == nullptr)
        {
            return;
        }
        const auto geometry = getProjectedGeometry(id);
        const auto targetSize = getEffectiveDeviceSize(device, orientation);
        std::vector<LogicalRect> obstacles;
        for (const auto &projected : responsiveLayout.nodes)
        {
            if (projected.id != id)
            {
                obstacles.push_back(
                    {projected.geometry.position, projected.geometry.size});
            }
        }
        const auto nearest =
            findNearestAvailablePosition(requestedPosition, geometry.size,
                                         targetSize, obstacles, shouldSnap);
        if (!nearest.has_value())
        {
            return;
        }
        auto movedGeometry = geometry;
        movedGeometry.position = *nearest;
        node->center = normalizedCenter(movedGeometry, targetSize);
        refreshResponsiveLayout();
        updateAllNodeBounds();
    }

    void moveSelection(LogicalPoint delta, const bool shouldSnap)
    {
        if (selectedIds.empty())
        {
            return;
        }

        struct SelectedGeometry
        {
            ArrangementNodeModel *node;
            ProjectedNodeGeometry geometry;
        };

        std::vector<SelectedGeometry> selected;
        selected.reserve(selectedIds.size());
        auto minimumX = std::numeric_limits<float>::max();
        auto minimumY = std::numeric_limits<float>::max();
        for (const auto id : selectedIds)
        {
            auto *node = findNode(id);
            const auto *geometry = findProjectedGeometry(responsiveLayout, id);
            if (node == nullptr || geometry == nullptr)
            {
                continue;
            }

            selected.push_back({node, *geometry});
            minimumX = std::min(minimumX, geometry->position.x);
            minimumY = std::min(minimumY, geometry->position.y);
        }
        if (selected.empty())
        {
            return;
        }

        // Snap only along the axis being moved. This prevents an arrow press
        // from unexpectedly correcting the selection on the other axis. Use
        // the next grid line in the requested direction when fine movement
        // has left the selection between grid lines.
        if (shouldSnap)
        {
            if (delta.x != 0.f)
            {
                delta.x = snapAxisTranslationToGrid(minimumX, delta.x,
                                                    arrangementGridSize);
            }
            if (delta.y != 0.f)
            {
                delta.y = snapAxisTranslationToGrid(minimumY, delta.y,
                                                    arrangementGridSize);
            }
        }

        std::vector<LogicalRect> obstacles;
        obstacles.reserve(responsiveLayout.nodes.size() - selected.size());
        for (const auto &projected : responsiveLayout.nodes)
        {
            if (!isSelected(projected.id))
            {
                obstacles.push_back(
                    {projected.geometry.position, projected.geometry.size});
            }
        }

        std::vector<LogicalRect> movingItems;
        movingItems.reserve(selected.size());
        for (const auto &item : selected)
        {
            movingItems.push_back({item.geometry.position, item.geometry.size});
        }
        const auto targetSize = getEffectiveDeviceSize(device, orientation);
        const auto availableTranslation = findNearestAvailableAxisTranslation(
            delta, movingItems, targetSize, obstacles,
            shouldSnap ? arrangementGridSize : 0.f);
        if (!availableTranslation.has_value())
        {
            return;
        }
        delta = *availableTranslation;

        for (auto &item : selected)
        {
            const LogicalPoint position{item.geometry.position.x + delta.x,
                                        item.geometry.position.y + delta.y};
            auto movedGeometry = item.geometry;
            movedGeometry.position = position;
            item.node->center = normalizedCenter(movedGeometry, targetSize);
        }
        refreshResponsiveLayout();
        updateAllNodeBounds();
        repaint();
    }

    void resizeNode(const std::uint64_t id, const ResizeCorner corner,
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
            corner == ResizeCorner::topLeft || corner == ResizeCorner::topRight
                ? -1.f
                : 1.f;
        const auto denominator =
            zoom * (referenceSize.width * referenceSize.width +
                    referenceSize.height * referenceSize.height);
        if (denominator <= 0.f)
        {
            return;
        }

        auto requestedScale = startGeometry.scale +
                              (static_cast<float>(screenDelta.x) *
                                   horizontalDirection * referenceSize.width +
                               static_cast<float>(screenDelta.y) *
                                   verticalDirection * referenceSize.height) /
                                  denominator;
        if (shouldSnap)
        {
            requestedScale = snapItemScaleToGrid(requestedScale, referenceSize);
        }
        const auto sharedScale = std::max(
            0.0001f,
            frozenResponsiveScale.value_or(responsiveLayout.sharedScale));
        const auto targetSize = getEffectiveDeviceSize(device, orientation);
        requestedScale =
            std::clamp(requestedScale, 0.01f,
                       targetSize.width * sharedScale / referenceSize.width);
        std::vector<LogicalRect> obstacles;
        for (const auto &projected : responsiveLayout.nodes)
        {
            if (projected.id != id)
            {
                obstacles.push_back(
                    {projected.geometry.position, projected.geometry.size});
            }
        }

        const auto makeGeometry = [&](const float projectedScale)
        {
            const LogicalSize requestedProjectedSize{
                referenceSize.width * projectedScale,
                referenceSize.height * projectedScale};
            const auto displayedPosition = positionForResizedNode(
                startGeometry, requestedProjectedSize, corner, useCentrePivot);
            return ProjectedNodeGeometry{
                displayedPosition, requestedProjectedSize, projectedScale};
        };

        auto requestedGeometry = makeGeometry(requestedScale);
        if (!isPlacementValid(
                {requestedGeometry.position, requestedGeometry.size},
                targetSize, obstacles))
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
                const auto geometry = makeGeometry(scale);
                if (isPlacementValid({geometry.position, geometry.size},
                                     targetSize, obstacles))
                {
                    lower = scale;
                }
                else
                {
                    upper = scale;
                }
            }
            requestedGeometry = makeGeometry(lower);
        }

        node->widthFraction =
            requestedGeometry.size.width / (targetSize.width * sharedScale);
        node->center = normalizedCenter(requestedGeometry, targetSize);
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
        const auto horizontal =
            projectPixelSpan(geometry.position.x, geometry.size.width, zoom,
                             surfaceBounds.getWidth());
        const auto vertical =
            projectPixelSpan(geometry.position.y, geometry.size.height, zoom,
                             surfaceBounds.getHeight());
        component.setBounds(surfaceBounds.getX() + horizontal.start,
                            surfaceBounds.getY() + vertical.start,
                            horizontal.end - horizontal.start,
                            vertical.end - vertical.start);
        component.resized();
    }

    void updateNodeBoundsById(const std::uint64_t id)
    {
        const auto found =
            std::find_if(nodeComponents.begin(), nodeComponents.end(),
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
    }

    DeviceProfile device;
    Orientation orientation = Orientation::portrait;
    ArrangementDocument document;
    ResponsiveLayout responsiveLayout;
    std::optional<float> frozenResponsiveScale;
    std::uint64_t nextNodeId = 1;
    float zoom = 1.f;
    juce::Rectangle<int> surfaceBounds;
    std::vector<std::unique_ptr<ArrangementNode>> nodeComponents;
    std::vector<std::uint64_t> selectedIds;
    bool isDragOverSurface = false;
};

GuiLabComponent::GuiLabComponent(juce::PropertiesFile &settingsToUse)
    : settings(settingsToUse)
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

    const auto &catalog = getArrangementCatalog();
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
    restoreRecentDocument();
}

GuiLabComponent::~GuiLabComponent()
{
    paletteViewport.setViewedComponent(nullptr, false);
}

void GuiLabComponent::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff202523));
}

bool GuiLabComponent::keyPressed(const juce::KeyPress &key)
{
    const auto modifiers = key.getModifiers();
    const auto shortcutDown =
        modifiers.isCommandDown() || modifiers.isCtrlDown();
    if (!shortcutDown)
    {
        return false;
    }

    const auto keyCode = key.getKeyCode();
    if (keyCode == 'O' || keyCode == 'o')
    {
        if (loadButton.isEnabled())
        {
            chooseFileToLoad();
        }
        return true;
    }

    if (keyCode == 'S' || keyCode == 's')
    {
        if (saveSetupButton.isEnabled())
        {
            if (modifiers.isShiftDown())
            {
                chooseSetupToSave();
            }
            else
            {
                saveCurrentSetup();
            }
        }
        return true;
    }

    return false;
}

void GuiLabComponent::resized()
{
    auto bounds = getLocalBounds().reduced(18);
    auto headingBounds = bounds.removeFromTop(38);
    saveSetupButton.setBounds(headingBounds.removeFromRight(104).reduced(0, 4));
    headingBounds.removeFromRight(8);
    saveButton.setBounds(headingBounds.removeFromRight(72).reduced(0, 4));
    headingBounds.removeFromRight(8);
    loadButton.setBounds(headingBounds.removeFromRight(72).reduced(0, 4));
    headingBounds.removeFromRight(8);
    heading.setBounds(headingBounds);
    bounds.removeFromTop(6);

    auto slotBounds = bounds.removeFromTop(34);
    slotsLabel.setBounds(slotBounds.removeFromLeft(44));
    for (auto &button : slotButtons)
    {
        button.setBounds(slotBounds.removeFromLeft(48).reduced(2, 2));
    }
    slotBounds.removeFromLeft(8);
    clearSlotButton.setBounds(slotBounds.removeFromLeft(92).reduced(0, 2));
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
    styleControlLabel(slotsLabel, "Slots");
    styleControlLabel(brandLabel, "Brand");
    styleControlLabel(deviceLabel, "Device");
    styleControlLabel(orientationLabel, "Orientation");
    styleComboBox(brandSelector);
    styleComboBox(deviceSelector);
    styleComboBox(orientationSelector);

    for (auto *button :
         {&loadButton, &saveButton, &saveSetupButton, &clearSlotButton})
    {
        button->setColour(juce::TextButton::buttonColourId,
                          juce::Colour(0xff343a38));
        button->setColour(juce::TextButton::textColourOffId,
                          juce::Colour(0xffedf2ef));
        addAndMakeVisible(*button);
    }
    addAndMakeVisible(slotsLabel);
    for (std::size_t i = 0; i < slotButtons.size(); ++i)
    {
        auto &button = slotButtons[i];
        button.setClickingTogglesState(false);
        button.setColour(juce::TextButton::buttonColourId,
                         juce::Colour(0xff343a38));
        button.setColour(juce::TextButton::buttonOnColourId,
                         juce::Colour(0xff67b8de));
        button.setColour(juce::TextButton::textColourOffId,
                         juce::Colour(0xffedf2ef));
        button.setColour(juce::TextButton::textColourOnId,
                         juce::Colour(0xff202523));
        button.onClick = [this, i]
        {
            selectSlot(i);
        };
        addAndMakeVisible(button);
    }

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
        persistActiveSlot();
    };
    loadButton.onClick = [this]
    {
        chooseFileToLoad();
    };
    saveButton.onClick = [this]
    {
        chooseDesignToSave();
    };
    saveSetupButton.onClick = [this]
    {
        saveCurrentSetup();
    };
    clearSlotButton.onClick = [this]
    {
        clearActiveSlot();
    };
    updateSlotButtons();
    updateTarget();
}

void GuiLabComponent::chooseFileToLoad()
{
    const auto setupWasMostRecent =
        settings.getValue(recentDocumentKindKey) == setupDocumentKind;
    const auto currentFile =
        setupWasMostRecent ? currentSetupFile : currentDesignFile;
    const auto rememberedFile = getRememberedFile(
        setupWasMostRecent ? recentSetupFileKey : recentDesignFileKey);
    const auto initialFile = currentFile.getFullPathName().isNotEmpty()
                                 ? currentFile
                                 : rememberedFile;
    const auto initialLocation = initialFile.getFullPathName().isNotEmpty()
                                     ? initialFile.getParentDirectory()
                                     : juce::File::getSpecialLocation(
                                           juce::File::userDocumentsDirectory);
    const auto wildcard = juce::String("*.") + arrangementFileExtension +
                          ";*." + arrangementSetupFileExtension;
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load arrangement or setup", initialLocation, wildcard, true);
    setFileButtonsEnabled(false);
    juce::Component::SafePointer<GuiLabComponent> safeThis(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
                                 juce::FileBrowserComponent::canSelectFiles,
                             [safeThis](const juce::FileChooser &chooser)
                             {
                                 if (safeThis == nullptr)
                                 {
                                     return;
                                 }
                                 safeThis->setFileButtonsEnabled(true);
                                 const auto file = chooser.getResult();
                                 if (file.getFullPathName().isNotEmpty())
                                 {
                                     safeThis->loadFile(file);
                                 }
                             });
}

void GuiLabComponent::chooseDesignToSave()
{
    auto initialFile = currentDesignFile;
    if (initialFile.getFullPathName().isEmpty())
    {
        initialFile = getRememberedFile(recentDesignFileKey);
    }
    if (initialFile.getFullPathName().isEmpty())
    {
        initialFile =
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                .getChildFile(juce::String("Untitled.") +
                              arrangementFileExtension);
    }
    else if (!initialFile.hasFileExtension(arrangementFileExtension))
    {
        initialFile = initialFile.withFileExtension(arrangementFileExtension);
    }
    const auto wildcard = juce::String("*.") + arrangementFileExtension;
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save arrangement design", initialFile, wildcard, true);
    setFileButtonsEnabled(false);
    juce::Component::SafePointer<GuiLabComponent> safeThis(this);
    fileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::canSelectFiles |
            juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis](const juce::FileChooser &chooser)
        {
            if (safeThis == nullptr)
            {
                return;
            }
            safeThis->setFileButtonsEnabled(true);
            const auto file = chooser.getResult();
            if (file.getFullPathName().isNotEmpty())
            {
                safeThis->saveDesignFile(file);
            }
        });
}

void GuiLabComponent::loadDesignFile(const juce::File &file)
{
    if (!file.existsAsFile())
    {
        showFileError("The selected design file does not exist.");
        return;
    }

    std::string errorMessage;
    const auto document = deserializeArrangementDocument(
        file.loadFileAsString().toStdString(), errorMessage);
    if (!document.has_value())
    {
        showFileError(errorMessage);
        return;
    }
    if (!workspace->loadDocument(*document, errorMessage))
    {
        showFileError(errorMessage);
        return;
    }

    currentDesignFile = file;
    rememberRecentFile(file, false);
    persistActiveSlot(true);
}

void GuiLabComponent::saveDesignFile(juce::File file)
{
    if (!file.hasFileExtension(arrangementFileExtension))
    {
        file = file.withFileExtension(arrangementFileExtension);
    }
    try
    {
        const auto contents =
            serializeArrangementDocument(workspace->getDocument());
        if (!file.replaceWithText(juce::String::fromUTF8(
                contents.data(), static_cast<int>(contents.size()))))
        {
            showFileError("The design could not be written to disk.");
            return;
        }
        currentDesignFile = file;
        rememberRecentFile(file, false);
        persistActiveSlot();
    }
    catch (const std::exception &error)
    {
        showFileError(juce::String("The design could not be saved: ") +
                      error.what());
    }
}

void GuiLabComponent::loadFile(const juce::File &file)
{
    if (file.hasFileExtension(arrangementFileExtension))
    {
        loadDesignFile(file);
    }
    else if (file.hasFileExtension(arrangementSetupFileExtension))
    {
        loadSetupFile(file);
    }
    else
    {
        showFileError(juce::String("Select a .") + arrangementFileExtension +
                      " arrangement or ." + arrangementSetupFileExtension +
                      " arrangement setup.");
    }
}

void GuiLabComponent::saveCurrentSetup()
{
    if (currentSetupFile.getFullPathName().isNotEmpty())
    {
        saveSetupFile(currentSetupFile);
    }
    else
    {
        chooseSetupToSave();
    }
}

void GuiLabComponent::chooseSetupToSave()
{
    auto initialFile = currentSetupFile;
    if (initialFile.getFullPathName().isEmpty())
    {
        initialFile = getRememberedFile(recentSetupFileKey);
    }
    if (initialFile.getFullPathName().isEmpty())
    {
        initialFile =
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                .getChildFile(juce::String("arrangement-setup.") +
                              arrangementSetupFileExtension);
    }
    else if (!initialFile.hasFileExtension(arrangementSetupFileExtension))
    {
        initialFile =
            initialFile.withFileExtension(arrangementSetupFileExtension);
    }
    const auto wildcard = juce::String("*.") + arrangementSetupFileExtension;
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save arrangement setup", initialFile, wildcard, true);
    setFileButtonsEnabled(false);
    juce::Component::SafePointer<GuiLabComponent> safeThis(this);
    fileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::canSelectFiles |
            juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis](const juce::FileChooser &chooser)
        {
            if (safeThis == nullptr)
            {
                return;
            }
            safeThis->setFileButtonsEnabled(true);
            const auto file = chooser.getResult();
            if (file.getFullPathName().isNotEmpty())
            {
                safeThis->saveSetupFile(file);
            }
        });
}

void GuiLabComponent::loadSetupFile(const juce::File &file)
{
    if (!file.existsAsFile())
    {
        showFileError("The selected setup file does not exist.");
        return;
    }
    std::string errorMessage;
    const auto loaded = deserializeArrangementSetup(
        file.loadFileAsString().toStdString(), errorMessage);
    if (!loaded.has_value())
    {
        showFileError(errorMessage);
        return;
    }

    setup = *loaded;
    activeSlot = findFirstOccupiedSlot(setup).value_or(0);
    const auto &slot = setup.slots[activeSlot];
    const auto orientation =
        slot.has_value() ? slot->orientation : Orientation::portrait;
    orientationSelector.setSelectedId(orientation == Orientation::landscape ? 2
                                                                            : 1,
                                      juce::dontSendNotification);
    const ArrangementDocument empty;
    if (!workspace->loadDocument(slot.has_value() ? slot->arrangement : empty,
                                 errorMessage))
    {
        showFileError(errorMessage);
        return;
    }
    currentSetupFile = file;
    currentDesignFile = juce::File();
    rememberRecentFile(file, true);
    updateTarget();
    updateSlotButtons();
}

void GuiLabComponent::saveSetupFile(juce::File file)
{
    if (!file.hasFileExtension(arrangementSetupFileExtension))
    {
        file = file.withFileExtension(arrangementSetupFileExtension);
    }
    persistActiveSlot();
    try
    {
        const auto contents = serializeArrangementSetup(setup);
        if (!file.replaceWithText(juce::String::fromUTF8(
                contents.data(), static_cast<int>(contents.size()))))
        {
            showFileError("The setup could not be written to disk.");
            return;
        }
        currentSetupFile = file;
        rememberRecentFile(file, true);
    }
    catch (const std::exception &error)
    {
        showFileError(juce::String("The setup could not be saved: ") +
                      error.what());
    }
}

void GuiLabComponent::restoreRecentDocument()
{
    const auto kind = settings.getValue(recentDocumentKindKey);
    const auto file = getRememberedFile(
        kind == setupDocumentKind ? recentSetupFileKey : recentDesignFileKey);
    if (!file.existsAsFile())
    {
        return;
    }

    if (kind == setupDocumentKind &&
        file.hasFileExtension(arrangementSetupFileExtension))
    {
        loadSetupFile(file);
    }
    else if (kind == designDocumentKind &&
             file.hasFileExtension(arrangementFileExtension))
    {
        loadDesignFile(file);
    }
}

juce::File GuiLabComponent::getRememberedFile(const juce::String &key) const
{
    const auto path = settings.getValue(key);
    return path.isNotEmpty() ? juce::File(path) : juce::File();
}

void GuiLabComponent::rememberRecentFile(const juce::File &file,
                                         const bool isSetup)
{
    settings.setValue(isSetup ? recentSetupFileKey : recentDesignFileKey,
                      file.getFullPathName());
    settings.setValue(recentDocumentKindKey,
                      isSetup ? setupDocumentKind : designDocumentKind);
    settings.saveIfNeeded();
}

void GuiLabComponent::persistActiveSlot(const bool replaceIdentity)
{
    const auto document = workspace->getDocument();
    if (document.nodes.empty())
    {
        setup.slots[activeSlot] = std::nullopt;
    }
    else
    {
        const auto existingId = setup.slots[activeSlot].has_value()
                                    ? setup.slots[activeSlot]->id
                                    : std::string{};
        ArrangementSlot slot;
        slot.id = replaceIdentity || existingId.empty() ? createArrangementId()
                                                        : existingId;
        slot.orientation = orientationSelector.getSelectedId() == 2
                               ? Orientation::landscape
                               : Orientation::portrait;
        slot.arrangement = document;
        setup.slots[activeSlot] = std::move(slot);
    }
    updateSlotButtons();
}

void GuiLabComponent::selectSlot(const std::size_t index)
{
    if (index >= setup.slots.size() || index == activeSlot)
    {
        return;
    }
    persistActiveSlot();
    activeSlot = index;
    const auto &slot = setup.slots[activeSlot];
    const auto orientation =
        slot.has_value() ? slot->orientation : Orientation::portrait;
    orientationSelector.setSelectedId(orientation == Orientation::landscape ? 2
                                                                            : 1,
                                      juce::dontSendNotification);
    std::string errorMessage;
    const ArrangementDocument empty;
    if (!workspace->loadDocument(slot.has_value() ? slot->arrangement : empty,
                                 errorMessage))
    {
        showFileError(errorMessage);
    }
    currentDesignFile = juce::File();
    updateTarget();
    updateSlotButtons();
}

void GuiLabComponent::clearActiveSlot()
{
    std::string errorMessage;
    if (!workspace->loadDocument({}, errorMessage))
    {
        showFileError(errorMessage);
        return;
    }
    setup.slots[activeSlot] = std::nullopt;
    currentDesignFile = juce::File();
    updateSlotButtons();
}

void GuiLabComponent::updateSlotButtons()
{
    for (std::size_t i = 0; i < slotButtons.size(); ++i)
    {
        slotButtons[i].setToggleState(i == activeSlot,
                                      juce::dontSendNotification);
        const auto occupied = setup.slots[i].has_value() &&
                              !setup.slots[i]->arrangement.nodes.empty();
        slotButtons[i].setButtonText(juce::String(static_cast<int>(i + 1)) +
                                     (occupied ? " •" : ""));
    }
}

void GuiLabComponent::setFileButtonsEnabled(const bool enabled)
{
    for (auto *button : {&loadButton, &saveButton, &saveSetupButton})
    {
        button->setEnabled(enabled);
    }
}

void GuiLabComponent::showFileError(const juce::String &message)
{
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, "Arrangement design", message,
        "OK", this);
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
