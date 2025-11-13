#include "Pad.hpp"

#include "controller/ClientEventController.hpp"
#include "eventregistry/EventRegistry.hpp"
#include "hardware/Component.hpp"

#include <Mpc.hpp>
#include "file/AkaiName.hpp"
#include <juce_graphics/juce_graphics.h>
#include "sampler/Sampler.hpp"
#include "sequencer/Sequencer.hpp"
#include "utils/TimeUtils.hpp"

#include <sequencer/Track.hpp>
#include <sequencer/Bus.hpp>

#include <sampler/Pad.hpp>
#include <sampler/NoteParameters.hpp>

#include <disk/SoundLoader.hpp>
#include <disk/MpcFile.hpp>

#include <lcdgui/screens/window/VmpcConvertAndLoadWavScreen.hpp>

#include <StrUtil.hpp>

using namespace vmpc_juce::gui::vector;
using namespace mpc::disk;
using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens::window;
using namespace mpc::lcdgui::screens::dialog2;
using namespace mpc::sequencer;
using namespace mpc::eventregistry;

Pad::Pad(Component *commonParentWithShadowToUse,
         const float shadowSizeToUse,
         const std::function<float()> &getScaleToUse, mpc::Mpc &mpcToUse,
         std::shared_ptr<mpc::hardware::Pad> mpcPadToUse)
    : SvgComponent({"pad.svg", "pressed_pad.svg"}, commonParentWithShadowToUse,
                   shadowSizeToUse, getScaleToUse),
      mpc(mpcToUse), mpcPad(std::move(mpcPadToUse)), getScale(getScaleToUse)
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
        const auto lower = mpc::StrUtil::toLower(s.toStdString());
        if (mpc::StrUtil::hasEnding(lower, ".snd") ||
            mpc::StrUtil::hasEnding(lower, ".wav"))
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
    const auto lower = mpc::StrUtil::toLower(path.toStdString());
    if (!mpc::StrUtil::hasEnding(lower, ".snd") &&
        !mpc::StrUtil::hasEnding(lower, ".wav"))
    {
        return;
    }

    auto sampler = mpc.getSampler();

    SoundLoader soundLoader(mpc, false);
    soundLoader.setPreview(false);

    auto compatiblePath =
        mpc::StrUtil::replaceAll(path.toStdString(), '\\', std::string("\\"));
    auto file = std::make_shared<MpcFile>(fs::path(compatiblePath));
    auto layeredScreen = mpc.getLayeredScreen();

    SoundLoaderResult result;
    auto sound = mpc.getSampler()->addSound();
    if (!sound)
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
                mpc.screens
                    ->get<ScreenId::VmpcConvertAndLoadWavScreen>();
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
        }
        else if (mpc::file::AkaiName::isValidChar(c))
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

    soundFileName = sampler->addOrIncreaseNumber(soundFileName);
    sound->setName(soundFileName);

    auto ext = file->getExtension();

    mpc.getLayeredScreen()->showPopupForMs(
        "LOADING " + mpc::StrUtil::padRight(soundFileName, " ", 16) + ext, 300);

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

    if (auto noteParams = program->getNoteParameters(padNote))
    {
        noteParams->setSoundIndex(soundIndex);
    }
}

void Pad::filesDropped(const juce::StringArray &files, int, int)
{
    if (files.size() != 1)
    {
        return;
    }

    const bool shouldBeConverted = false;
    loadFile(files[0], shouldBeConverted);
}

int Pad::getVelo(int veloY)
{
    return static_cast<int>(static_cast<float>(veloY) /
                            static_cast<float>(getHeight()));
}

void Pad::mouseDrag(const juce::MouseEvent &event)
{
    if (!mpcPad->isPressed())
    {
        return;
    }

    mpcPad->aftertouch(static_cast<unsigned char>(getVelo(event.y)));
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
    static constexpr float immediateDecay = 0.03f;
    static constexpr float immediateFadeFactorPrimary = 0.8f;
    static constexpr float immediateFadeFactorNonPrimary = 0.6f;
    static constexpr float decayThreshold = 0.f;

    auto decayPress = [&](std::optional<Press> &press, bool isPrimary) -> bool
    {
        if (!press)
        {
            return false;
        }

        if (press->alpha == 1.f && !press->wasPaintedWithInitialAlpha)
        {
            return false;
        }

        bool mutated = false;
        const float immediateFadeFactorToUse =
            (isPrimary ? immediateFadeFactorPrimary
                       : immediateFadeFactorNonPrimary);
        switch (press->phase)
        {
            case Press::Phase::Immediate:
                press->alpha -= immediateDecay;
                if (press->alpha <= immediateFadeFactorToUse)
                {
                    press->alpha = immediateFadeFactorToUse;
                    press->phase = Press::Phase::Sustained;
                }
                mutated = true;
                break;

            case Press::Phase::Sustained:
                break;

            case Press::Phase::Releasing:
                press->alpha -= decay;
                if (press->alpha <= decayThreshold)
                {
                    press.reset();
                    return true;
                }
                mutated = true;
                break;
        }
        return mutated;
    };

    const int bank =
        static_cast<int>(mpc.clientEventController->getActiveBank());
    const int padIndexWithBank = mpcPad->getIndex() + (bank * 16);

    bool mutated = false;

    if (mpcPad->isPressed())
    {
        auto veloOrPressure =
            mpcPad->getPressure().value_or(mpcPad->getVelocity().value());

        if (!primaryPress || primaryPress->phase == Press::Phase::Releasing)
        {
            primaryPress = Press{padIndexWithBank, 1.f, veloOrPressure,
                                 Press::Phase::Immediate, mpc::utils::nowInMilliseconds()};
            mutated = true;
        }
        else
        {
            if (primaryPress->phase == Press::Phase::Immediate)
            {
                mutated |= decayPress(primaryPress, true);
            }

            if (primaryPress->veloOrPressure != veloOrPressure)
            {
                primaryPress->veloOrPressure = veloOrPressure;
                mutated = true;
            }
        }
    }
    else if (primaryPress)
    {
        if (primaryPress->phase != Press::Phase::Releasing)
        {
            primaryPress->phase = Press::Phase::Releasing;
        }
        mutated |= decayPress(primaryPress, true);
    }

    const auto snapshot = mpc.eventRegistry->getSnapshot();
    static const std::vector exclude{
        Source::VirtualMpcHardware};

    const auto mostRecentPress =
        snapshot.getMostRecentProgramPadPress(
            static_cast<uint8_t>(padIndexWithBank), exclude);

    if (mostRecentPress)
    {
        auto veloOrPressure = snapshot.getPressedProgramPadAfterTouchOrVelocity(
            static_cast<uint8_t>(padIndexWithBank));

        if (!secondaryPress ||
            secondaryPress->phase == Press::Phase::Releasing ||
            secondaryPress->pressTime != mostRecentPress->pressTimeMs)
        {
            secondaryPress =
                Press{padIndexWithBank, 1.f, veloOrPressure,
                      Press::Phase::Immediate, mostRecentPress->pressTimeMs};
            mutated = true;
        }
        else
        {
            mutated |= decayPress(secondaryPress, false);
        }

        if (secondaryPress->veloOrPressure != veloOrPressure)
        {
            secondaryPress->veloOrPressure = veloOrPressure;
            mutated = true;
        }
    }
    else if (secondaryPress)
    {
        if (secondaryPress->phase != Press::Phase::Releasing)
        {
            secondaryPress->phase = Press::Phase::Releasing;
        }
        mutated |= decayPress(secondaryPress, false);
    }

    int otherBanked = -1;

    std::optional<ProgramPadPressEvent> otherBankedPress;

    for (int i = mpcPad->getIndex(); i < 64; i += 16)
    {
        if (i == padIndexWithBank)
        {
            continue;
        }

        if (auto mostRecentOtherBankedPress =
                snapshot.getMostRecentProgramPadPress(static_cast<uint8_t>(i),
                                                      exclude);
            mostRecentOtherBankedPress)
        {
            otherBanked = i;
            otherBankedPress = mostRecentOtherBankedPress;
            break;
        }
    }

    if (otherBanked != -1 && otherBankedPress)
    {
        auto veloOrPressure = snapshot.getPressedProgramPadAfterTouchOrVelocity(
            static_cast<uint8_t>(otherBanked));

        if (!tertiaryPress || tertiaryPress->phase == Press::Phase::Releasing ||
            tertiaryPress->pressTime != otherBankedPress->pressTimeMs)
        {
            tertiaryPress =
                Press{otherBanked, 1.f, veloOrPressure, Press::Phase::Immediate,
                      otherBankedPress->pressTimeMs};
            mutated = true;
        }
        else
        {
            mutated |= decayPress(tertiaryPress, false);
        }

        if (tertiaryPress->veloOrPressure != veloOrPressure)
        {
            tertiaryPress->veloOrPressure = veloOrPressure;
            mutated = true;
        }
    }
    else if (tertiaryPress)
    {
        if (tertiaryPress->phase != Press::Phase::Releasing)
        {
            tertiaryPress->phase = Press::Phase::Releasing;
        }
        mutated |= decayPress(tertiaryPress, false);
    }

    if (bank != lastBank)
    {
        lastBank = bank;
        auto fadePress = [&](std::optional<Press> &press)
        {
            if (press && press->alpha == 1.f)
            {
                press->alpha -= decay;
                mutated = true;
            }
        };
        fadePress(secondaryPress);
        fadePress(tertiaryPress);
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
    if (primaryPress)
    {
        glowSvg->setAlpha(
            std::clamp(primaryPress->getAlphaWithVeloApplied(), 0.f, 1.f));

        if (primaryPress->alpha == 1.f &&
            !primaryPress->wasPaintedWithInitialAlpha)
        {
            primaryPress->wasPaintedWithInitialAlpha = true;
        }
    }
    else
    {
        glowSvg->setAlpha(0.f);
    }

    SvgComponent::paint(g);
    const float scale = getScale();

    auto drawOutline = [&](std::optional<Press> &press, juce::Colour baseColour,
                           float inset, float baseThickness,
                           float baseCornerRadius, bool addInwardGlow = true)
    {
        if (!press)
        {
            return;
        }

        if (press->alpha == 1.f && !press->wasPaintedWithInitialAlpha)
        {
            press->wasPaintedWithInitialAlpha = true;
        }

        const float alpha =
            std::clamp(press->getAlphaWithVeloApplied(), 0.f, 1.f);

        if (alpha <= 0.f)
        {
            return;
        }

        const float thickness = baseThickness * scale;
        const float cornerRadius = baseCornerRadius * scale;
        const auto bounds =
            getLocalBounds().reduced(static_cast<int>(inset * scale)).toFloat();
        const float baseAlpha = alpha * baseColour.getFloatAlpha();

        if (addInwardGlow)
        {
            juce::Colour inner = baseColour.withAlpha(baseAlpha * 0.05f);
            juce::Colour outer = baseColour.withAlpha(baseAlpha * 0.5f);

            juce::ColourGradient grad(
                inner, bounds.getCentreX(), bounds.getCentreY(), outer,
                bounds.getCentreX(),
                bounds.getCentreY() - bounds.getHeight() * 1.2f, true);

            grad.addColour(0.6, baseColour.withAlpha(baseAlpha * 0.55f));
            grad.addColour(1.0, outer);

            g.setGradientFill(grad);
            g.fillRoundedRectangle(bounds, cornerRadius);
        }

        g.setColour(baseColour.withAlpha(baseAlpha));
        g.drawRoundedRectangle(bounds, cornerRadius, thickness);
    };

    static const auto secondaryColor =
        juce::Colour::fromFloatRGBA(0.95f, 0.65f, 0.25f, 1.f);
    static const auto tertiaryColor =
        juce::Colour::fromFloatRGBA(0.35f, 0.55f, 1.f, 1.f);

    drawOutline(secondaryPress, secondaryColor, 2.65f, 1.5f, .4f, true);
    drawOutline(tertiaryPress, tertiaryColor, 2.65f, 1.5f, .4f, true);
}
