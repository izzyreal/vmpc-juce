#include "GuiLabComponent.hpp"

#include "PreviewViewUtil.hpp"
#include "VmpcJuceResourceUtil.hpp"
#include "gui/vector/Constants.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <exception>

using namespace vmpc_juce::guilab;
using vmpc_juce::gui::vector::Constants;

namespace
{
    constexpr std::array<CatalogEntry, 18> catalog{{
        {"lcd-bare", "LCD - bare", "lcd_bare", 210, 55},
        {"lcd-mounted", "LCD - mounted", "lcd_mounted_compact", 230, 100},
        {"lcd-mounted-functions", "LCD - mounted + function buttons",
         "display_and_f_keys_compact", 230, 115},
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
        {"locate", "Locate", "locate_group", 179, 28},
        {"transport-horizontal", "Transport - horizontal", "transport_keys_lab",
         179, 30},
        {"transport-vertical", "Transport - vertical",
         "transport_keys_vertical", 45, 150},
        {"levels", "Full Level + 16 Levels", "full_level_16_levels", 72, 36},
        {"sequence-mute", "Next Seq + Track Mute", "next_seq_track_mute", 72,
         23},
        {"pads-banks", "Pads + Pad Bank", "pads_with_banks", 206, 236},
        {"gain-volume", "Rec Gain + Main Volume", "rec_gain_main_volume", 110,
         52},
    }};

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
    explicit PreviewCard(const CatalogEntry &entryToUse)
        : entry(entryToUse), preview(entry)
    {
        title.setText(entry.title, juce::dontSendNotification);
        title.setFont(juce::Font(15.f, juce::Font::bold));
        title.setColour(juce::Label::textColourId, juce::Colour(0xffe8ecea));
        title.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(title);
        addAndMakeVisible(preview);
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
    }

private:
    CatalogEntry entry;
    float scale = 2.f;
    juce::Label title;
    PreviewComponent preview;
};

GuiLabComponent::GuiLabComponent()
{
    heading.setText("VMPC2000XL arrangement lab", juce::dontSendNotification);
    heading.setFont(juce::Font(24.f, juce::Font::bold));
    heading.setColour(juce::Label::textColourId, juce::Colour(0xffedf2ef));
    heading.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(heading);

    viewport.setViewedComponent(&gallery, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setColour(juce::ScrollBar::thumbColourId,
                       juce::Colour(0xff68716e));
    addAndMakeVisible(viewport);

    cards.reserve(catalog.size());
    for (const auto &entry : catalog)
    {
        auto card = std::make_unique<PreviewCard>(entry);
        gallery.addAndMakeVisible(*card);
        cards.push_back(std::move(card));
    }
}

GuiLabComponent::~GuiLabComponent()
{
    viewport.setViewedComponent(nullptr, false);
}

void GuiLabComponent::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff202523));
}

void GuiLabComponent::resized()
{
    auto bounds = getLocalBounds().reduced(18);
    heading.setBounds(bounds.removeFromTop(38));
    bounds.removeFromTop(10);
    viewport.setBounds(bounds);
    layoutGallery();
}

void GuiLabComponent::layoutGallery()
{
    constexpr int gap = 14;
    const int availableWidth = std::max(1, viewport.getWidth() - 16);
    constexpr int widestHardwareWidth = 230;
    const float scale =
        std::min(2.f, static_cast<float>(availableWidth - gap * 2 - 24) /
                          static_cast<float>(widestHardwareWidth));

    int x = gap;
    int y = gap;
    int rowHeight = 0;

    for (auto &card : cards)
    {
        card->setHardwareScale(scale);
        const int width = card->requiredWidth();
        const int height = card->requiredHeight();

        if (x != gap && x + width + gap > availableWidth)
        {
            x = gap;
            y += rowHeight + gap;
            rowHeight = 0;
        }

        card->setBounds(x, y, width, height);
        x += width + gap;
        rowHeight = std::max(rowHeight, height);
    }

    gallery.setSize(availableWidth, y + rowHeight + gap);
}
