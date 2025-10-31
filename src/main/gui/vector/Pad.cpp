#include "Pad.hpp"

#include "controller/ClientEventController.hpp"
#include "eventregistry/EventRegistry.hpp"
#include "hardware/Component.hpp"

#include <Mpc.hpp>
#include "file/AkaiName.hpp"
#include <juce_graphics/juce_graphics.h>
#include "lcdgui/screens/DrumScreen.hpp"

#include <sequencer/Track.hpp>
#include <sequencer/Bus.hpp>

#include <sampler/Pad.hpp>
#include <sampler/NoteParameters.hpp>

#include <disk/SoundLoader.hpp>
#include <disk/MpcFile.hpp>

#include <lcdgui/screens/window/VmpcConvertAndLoadWavScreen.hpp>
#include <lcdgui/ScreenGroups.hpp>

#include <StrUtil.hpp>

#include <Logger.hpp>

using namespace vmpc_juce::gui::vector;
using namespace mpc::disk;
using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens::window;
using namespace mpc::lcdgui::screens::dialog2;
using namespace mpc::sequencer;

static int
getDrumIndexForCurrentScreen(mpc::Mpc &mpc,
                             const std::shared_ptr<ScreenComponent> screen)
{
    const bool isSamplerScreen = screengroups::isSamplerScreen(screen);
    return isSamplerScreen
               ? mpc.screens->get<mpc::lcdgui::screens::DrumScreen>()->getDrum()
               : mpc.getSequencer()->getActiveTrack()->getBus() - 1;
}

static std::shared_ptr<mpc::sampler::Program>
getProgramForCurrentScreen(mpc::Mpc &mpc)
{
    const auto currentScreen = mpc.getLayeredScreen()->getCurrentScreen();
    const int drumIndex = getDrumIndexForCurrentScreen(mpc, currentScreen);
    if (drumIndex < 0)
    {
        return nullptr;
    }

    auto sampler = mpc.getSampler();
    return sampler->getProgram(
        mpc.getSequencer()->getDrumBus(drumIndex)->getProgram());
}

Pad::Pad(juce::Component *commonParentWithShadowToUse,
         const float shadowSizeToUse,
         const std::function<float()> &getScaleToUse, mpc::Mpc &mpcToUse,
         std::shared_ptr<mpc::hardware::Pad> mpcPadToUse)
    : SvgComponent({"pad.svg", "pressed_pad.svg"}, commonParentWithShadowToUse,
                   shadowSizeToUse, getScaleToUse),
      mpc(mpcToUse), mpcPad(mpcPadToUse), getScale(getScaleToUse)
{
    glowSvg = new SvgComponent({"pad_glow.svg"}, commonParentWithShadowToUse,
                               0.f, getScaleToUse);
    glowSvg->setAlpha(0.f);
    addAndMakeVisible(glowSvg);
}

bool Pad::isInterestedInFileDrag(const juce::StringArray &files)
{
    if (files.size() != 1)
    {
        return false;
    }

    for (auto &s : files)
    {
        if (mpc::StrUtil::hasEnding(mpc::StrUtil::toLower(s.toStdString()),
                                    ".snd") ||
            mpc::StrUtil::hasEnding(mpc::StrUtil::toLower(s.toStdString()),
                                    ".wav"))
        {
            if (glowSvg->getAlpha() == 0.f)
            {
                glowSvg->setAlpha(1.f);
                repaint();
            }
            return true;
        }
    }
    return false;
}

void Pad::loadFile(const juce::String path, bool shouldBeConverted)
{
    if (mpc::StrUtil::hasEnding(mpc::StrUtil::toLower(path.toStdString()),
                                ".snd") ||
        mpc::StrUtil::hasEnding(mpc::StrUtil::toLower(path.toStdString()),
                                ".wav"))
    {
        auto sampler = mpc.getSampler();

        SoundLoader soundLoader(mpc, false);
        soundLoader.setPreview(false);

        auto compatiblePath = mpc::StrUtil::replaceAll(path.toStdString(), '\\',
                                                       std::string("\\"));

        auto file =
            std::make_shared<mpc::disk::MpcFile>(fs::path(compatiblePath));

        auto layeredScreen = mpc.getLayeredScreen();

        SoundLoaderResult result;
        auto sound = mpc.getSampler()->addSound();

        if (sound == nullptr)
        {
            return;
        }

        soundLoader.loadSound(file, result, sound, shouldBeConverted);

        if (!result.success)
        {
            sampler->deleteSound(sound);

            if (result.canBeConverted)
            {
                auto loadRoutine = [&, path, layeredScreen]()
                {
                    const bool shouldBeConverted2 = true;
                    loadFile(path, shouldBeConverted2);
                };

                auto convertAndLoadWavScreen =
                    mpc.screens->get<VmpcConvertAndLoadWavScreen>();
                convertAndLoadWavScreen->setLoadRoutine(loadRoutine);
                layeredScreen->openScreen("vmpc-convert-and-load-wav");
            }
            return;
        }

        std::string soundFileName;

        for (auto &c : mpc::StrUtil::toUpper(file->getNameWithoutExtension()))
        {
            if (c == ' ')
            {
                soundFileName.push_back('_');
                continue;
            }
            if (mpc::file::AkaiName::isValidChar(c))
            {
                soundFileName.push_back(c);
            }
        }

        if (soundFileName.empty())
        {
            return;
        }

        if (soundFileName.length() >= 16)
        {
            soundFileName = soundFileName.substr(0, 16);
        }

        /**
         * TODO Pad::loadFile should use the exact same logic as LoadScreen does
         * when loading SND or WAV
         * files. For now we preemptively bump the sound counter to make sure
         * we don't get duplicate sound names. But if we follow LoadScreen
         * logic, we will get a chance to CANCEL or RENAME in case a sound
         * already exists. With the current logic, loading a file named
         * "FULL1.SND" into an empty VMPC2000XL results in a sound named FULL2.
         */
        soundFileName = sampler->addOrIncreaseNumber(soundFileName);
        sound->setName(soundFileName);

        auto ext = file->getExtension();

        mpc.getLayeredScreen()->showPopupForMs(
            "LOADING " + mpc::StrUtil::padRight(soundFileName, " ", 16) + ext,
            300);

        auto drumBus = mpc.getSequencer()->getBus<DrumBus>(
            mpc.getSequencer()->getActiveTrack()->getBus());

        if (!drumBus)
        {
            return;
        }

        auto programIndex = drumBus->getProgram();
        auto program = mpc.getSampler()->getProgram(programIndex);
        auto soundIndex = mpc.getSampler()->getSoundCount() - 1;
        const int bank =
            static_cast<int>(mpc.clientEventController->getActiveBank());
        auto padIndex = mpcPad->getIndex() + (bank * 16);
        auto programPad = program->getPad(padIndex);
        auto padNote = programPad->getNote();

        auto noteParameters = dynamic_cast<mpc::sampler::NoteParameters *>(
            program->getNoteParameters(padNote));

        if (noteParameters != nullptr)
        {
            noteParameters->setSoundIndex(soundIndex);
        }
    }
}

void Pad::filesDropped(const juce::StringArray &files, int, int)
{
    if (files.size() != 1)
    {
        return;
    }

    const bool shouldBeConverted = false;

    for (auto &f : files)
    {
        loadFile(f, shouldBeConverted);
    }
}

int Pad::getVelo(int veloY)
{
    return (float)veloY / getHeight();
}

void Pad::mouseDrag(const juce::MouseEvent &event)
{
    if (!mpcPad->isPressed())
    {
        return;
    }

    auto newVelo = getVelo(event.y);

    mpcPad->aftertouch(static_cast<unsigned char>(newVelo));
}

void Pad::resized()
{
    SvgComponent::resized();
    glowSvg->setBounds(0, 0, getWidth(), getHeight());
}

Pad::~Pad()
{
    delete glowSvg;
}

void Pad::sharedTimerCallback()
{
    static constexpr float decay = 0.03f;
    static constexpr float decayThreshold = 0.f;

    auto applyDecay = [&](auto &presses) -> bool
    {
        bool mutated = false;
        for (auto it = presses.begin(); it != presses.end();)
        {
            if (it->alpha <= decayThreshold)
            {
                it = presses.erase(it);
                mutated = true;
            }
            else
            {
                float oldAlpha = it->alpha;
                it->alpha -= decay;
                if (it->alpha != oldAlpha)
                {
                    mutated = true;
                }
                ++it;
            }
        }
        return mutated;
    };

    const int bank =
        static_cast<int>(mpc.clientEventController->getActiveBank());
    const int padIndexWithBank = mpcPad->getIndex() + (bank * 16);

    bool mutated = false;

    if (mpcPad->isPressed())
    {
        bool hasPrimaryPress =
            std::any_of(primaryPresses.begin(), primaryPresses.end(),
                        [](auto &p)
                        {
                            return p.alpha == 1.f;
                        });
        if (!hasPrimaryPress)
        {
            primaryPresses.push_back({padIndexWithBank, 1.f});
            mutated = true;
        }
    }
    else
    {
        mutated |= applyDecay(primaryPresses);
    }

    float primaryPressAlpha = 0.f;
    for (auto &p : primaryPresses)
    {
        primaryPressAlpha += p.alpha;
    }

    float newAlpha = std::clamp(primaryPressAlpha, 0.f, 1.f);
    if (glowSvg->getAlpha() != newAlpha)
    {
        glowSvg->setAlpha(newAlpha);
        mutated = true;
    }

    const auto snapshot = mpc.eventRegistry->getSnapshot();
    static const std::vector<mpc::eventregistry::Source> sourcesToExclude{
        mpc::eventregistry::Source::VirtualMpcHardware};

    if (auto program = getProgramForCurrentScreen(mpc))
    {
        const bool pressedVisible = snapshot.isProgramPadPressed(
            static_cast<uint8_t>(padIndexWithBank), sourcesToExclude);
        if (pressedVisible)
        {
            bool hasSecondaryPress =
                std::any_of(secondaryPresses.begin(), secondaryPresses.end(),
                            [&](auto &p)
                            {
                                return p.alpha == 1.f &&
                                       p.padIndexWithBank == padIndexWithBank;
                            });
            if (!hasSecondaryPress)
            {
                secondaryPresses.push_back({padIndexWithBank, 1.f});
                mutated = true;
            }
        }
        else
        {
            mutated |= applyDecay(secondaryPresses);
        }

        for (int i = mpcPad->getIndex(); i < 64; i += 16)
        {
            if (i == padIndexWithBank)
            {
                continue;
            }
            const bool pressedInvisible = snapshot.isProgramPadPressed(
                static_cast<uint8_t>(i), sourcesToExclude);

            if (pressedInvisible)
            {
                bool hasTertiaryPress = std::any_of(
                    tertiaryPresses.begin(), tertiaryPresses.end(),
                    [&](auto &p)
                    {
                        return p.alpha == 1.f && p.padIndexWithBank == i;
                    });
                if (!hasTertiaryPress)
                {
                    tertiaryPresses.push_back({i, 1.f});
                    mutated = true;
                }
            }
            else
            {
                mutated |= applyDecay(tertiaryPresses);
            }
        }
    }

    if (bank != lastBank)
    {
        lastBank = bank;
        auto fadeAll = [&](auto &presses)
        {
            for (auto &p : presses)
            {
                if (p.alpha == 1.f)
                {
                    p.alpha -= decay;
                    mutated = true;
                }
            }
        };
        fadeAll(secondaryPresses);
        fadeAll(tertiaryPresses);
    }

    if (mutated)
    {
        mutatedSinceLastPaint = true;
        fadeFrameCounter = 0;
    }

    if (mutatedSinceLastPaint)
    {
        if (mutated)
        {
            repaint();
            mutatedSinceLastPaint = false;
        }
        else
        {
            if (++fadeFrameCounter >= fadeRepaintInterval)
            {
                repaint();
                fadeFrameCounter = 0;
                mutatedSinceLastPaint = false;
            }
        }
    }
}

void Pad::paint(juce::Graphics &g)
{
    SvgComponent::paint(g);

    const float scale = getScale();

    auto drawOutline = [&](const std::vector<Press> &presses,
                           juce::Colour baseColour, int inset,
                           float baseThickness, float baseCornerRadius,
                           bool addInwardGlow = true)
    {
        float alpha = 0.f;
        for (auto &p : presses)
        {
            alpha += p.alpha;
        }
        alpha = std::clamp(alpha, 0.f, 1.f);
        if (alpha <= 0.f)
        {
            return;
        }

        const float thickness = baseThickness * scale;
        const float cornerRadius = baseCornerRadius * scale;
        const auto bounds = getLocalBounds().reduced(inset * scale).toFloat();
        const float baseAlpha = alpha * baseColour.getFloatAlpha();

        if (addInwardGlow)
        {
            // Create a radial gradient that’s bright at the edges, fading
            // inward.
            juce::Colour inner = baseColour.withAlpha(baseAlpha * 0.05f);
            juce::Colour outer = baseColour.withAlpha(baseAlpha * 0.5f);

            juce::ColourGradient grad(
                inner, bounds.getCentreX(), bounds.getCentreY(), outer,
                bounds.getCentreX(),
                bounds.getCentreY() -
                    bounds.getHeight() * 1.2f, // reduced radius
                true);

            grad.addColour(0.6, baseColour.withAlpha(baseAlpha * 0.55f));
            grad.addColour(1.0, outer);

            g.setGradientFill(grad);
            g.fillRoundedRectangle(bounds, cornerRadius);
        }

        // Outline on top for crisp definition
        g.setColour(
            baseColour.withAlpha(baseAlpha * baseColour.getFloatAlpha()));
        g.drawRoundedRectangle(bounds, cornerRadius, thickness);
    };

    static const auto secondaryColor = juce::Colour::fromFloatRGBA(
        0.95f, 0.65f, 0.25f, 1.f); // warm ochre-orange
    static const auto tertiaryColor = juce::Colour::fromFloatRGBA(
        0.3f, 0.5f, 0.95f, 0.6f); // cooler blue-purplish tone

    drawOutline(secondaryPresses, secondaryColor, 3, 3.0f, 1.0f, true);
    drawOutline(tertiaryPresses, tertiaryColor, 3, 3.0f, 1.0f, true);
}
