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
        return nullptr;

    auto sampler = mpc.getSampler();
    return sampler->getProgram(
        mpc.getSequencer()->getDrumBus(drumIndex)->getProgram());
}

Pad::Pad(juce::Component *commonParentWithShadowToUse,
         const float shadowSizeToUse,
         const std::function<float()> &getScaleToUse,
         mpc::Mpc &mpcToUse,
         std::shared_ptr<mpc::hardware::Pad> mpcPadToUse)
    : SvgComponent({"pad.svg", "pressed_pad.svg"}, commonParentWithShadowToUse,
                   shadowSizeToUse, getScaleToUse),
      mpc(mpcToUse),
      mpcPad(std::move(mpcPadToUse)),
      getScale(getScaleToUse)
{
    glowSvg = new SvgComponent({"pad_glow.svg"}, commonParentWithShadowToUse,
                               0.f, getScaleToUse);
    glowSvg->setAlpha(0.f);
    addAndMakeVisible(glowSvg);
}

bool Pad::isInterestedInFileDrag(const juce::StringArray &files)
{
    if (files.size() != 1)
        return false;

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
        return;

    auto sampler = mpc.getSampler();

    SoundLoader soundLoader(mpc, false);
    soundLoader.setPreview(false);

    auto compatiblePath =
        mpc::StrUtil::replaceAll(path.toStdString(), '\\', std::string("\\"));
    auto file = std::make_shared<mpc::disk::MpcFile>(fs::path(compatiblePath));
    auto layeredScreen = mpc.getLayeredScreen();

    SoundLoaderResult result;
    auto sound = mpc.getSampler()->addSound();
    if (!sound)
        return;

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
            soundFileName.push_back('_');
        else if (mpc::file::AkaiName::isValidChar(c))
            soundFileName.push_back(c);
    }

    if (soundFileName.empty())
        return;

    if (soundFileName.length() >= 16)
        soundFileName = soundFileName.substr(0, 16);

    soundFileName = sampler->addOrIncreaseNumber(soundFileName);
    sound->setName(soundFileName);

    auto ext = file->getExtension();

    mpc.getLayeredScreen()->showPopupForMs(
        "LOADING " + mpc::StrUtil::padRight(soundFileName, " ", 16) + ext, 300);

    auto drumBus = mpc.getSequencer()->getBus<DrumBus>(
        mpc.getSequencer()->getActiveTrack()->getBus());
    if (!drumBus)
        return;

    auto programIndex = drumBus->getProgram();
    auto program = mpc.getSampler()->getProgram(programIndex);
    auto soundIndex = mpc.getSampler()->getSoundCount() - 1;
    const int bank = static_cast<int>(mpc.clientEventController->getActiveBank());
    auto padIndex = mpcPad->getIndex() + (bank * 16);
    auto programPad = program->getPad(padIndex);
    auto padNote = programPad->getNote();

    if (auto noteParams =
            dynamic_cast<mpc::sampler::NoteParameters *>(
                program->getNoteParameters(padNote)))
    {
        noteParams->setSoundIndex(soundIndex);
    }
}

void Pad::filesDropped(const juce::StringArray &files, int, int)
{
    if (files.size() != 1)
        return;

    const bool shouldBeConverted = false;
    loadFile(files[0], shouldBeConverted);
}

int Pad::getVelo(int veloY)
{
    return static_cast<float>(veloY) / getHeight();
}

void Pad::mouseDrag(const juce::MouseEvent &event)
{
    if (!mpcPad->isPressed())
        return;

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
            return false;

        bool mutated = false;
        const float immediateFadeFactorToUse = (isPrimary ? immediateFadeFactorPrimary : immediateFadeFactorNonPrimary);
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
        auto veloOrPressure = mpcPad->getPressure().value_or(mpcPad->getVelocity().value());

        if (!primaryPress || primaryPress->phase == Press::Phase::Releasing)
        {
            primaryPress = Press{padIndexWithBank, 1.f, veloOrPressure, Press::Phase::Immediate };
            mutated = true;
        }
        else
        {
            if (primaryPress->phase == Press::Phase::Immediate)
            {
                if (primaryPress->veloOrPressure != veloOrPressure)
                {
                    primaryPress->veloOrPressure = veloOrPressure;
                    mutated = true;
                }
                mutated |= decayPress(primaryPress, true);
            }
        }
    }
    else if (primaryPress)
    {
        if (primaryPress->phase != Press::Phase::Releasing)
            primaryPress->phase = Press::Phase::Releasing;
        mutated |= decayPress(primaryPress, true);
    }

    const auto snapshot = mpc.eventRegistry->getSnapshot();
    static const std::vector<mpc::eventregistry::Source> exclude{
        mpc::eventregistry::Source::VirtualMpcHardware};

    if (auto program = getProgramForCurrentScreen(mpc))
    {
        const bool pressedVisible =
            snapshot.isProgramPadPressed(static_cast<uint8_t>(padIndexWithBank),
                                         exclude);

        if (pressedVisible)
        {
            auto veloOrPressure = snapshot.getPressedProgramPadAfterTouchOrVelocity(static_cast<uint8_t>(padIndexWithBank));

            if (!secondaryPress || secondaryPress->phase == Press::Phase::Releasing)
            {
                secondaryPress = Press{padIndexWithBank, 1.f, veloOrPressure, Press::Phase::Immediate};
                mutated = true;
            }
            else
            {
                if (secondaryPress->veloOrPressure != veloOrPressure)
                {
                    secondaryPress->veloOrPressure = veloOrPressure;
                    mutated = true;
                }
                mutated |= decayPress(secondaryPress, false);
            }
        }
        else if (secondaryPress)
        {
            if (secondaryPress->phase != Press::Phase::Releasing)
                secondaryPress->phase = Press::Phase::Releasing;
            mutated |= decayPress(secondaryPress, false);
        }

        int otherBanked = -1;
        for (int i = mpcPad->getIndex(); i < 64; i += 16)
        {
            if (i == padIndexWithBank)
                continue;
            if (snapshot.isProgramPadPressed(static_cast<uint8_t>(i), exclude))
            {
                otherBanked = i;
                break;
            }
        }

        if (otherBanked != -1)
        {
            auto veloOrPressure = snapshot.getPressedProgramPadAfterTouchOrVelocity(static_cast<uint8_t>(otherBanked));

            if (!tertiaryPress || tertiaryPress->phase == Press::Phase::Releasing)
            {
                tertiaryPress = Press{otherBanked, 1.f, veloOrPressure, Press::Phase::Immediate};
                mutated = true;
            }
            else
            {
                if (tertiaryPress->veloOrPressure != veloOrPressure)
                {
                    tertiaryPress->veloOrPressure = veloOrPressure;
                    mutated = true;
                }

                mutated |= decayPress(tertiaryPress, false);
            }
        }
        else if (tertiaryPress)
        {
            if (tertiaryPress->phase != Press::Phase::Releasing)
                tertiaryPress->phase = Press::Phase::Releasing;
            mutated |= decayPress(tertiaryPress, false);
        }
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
        printf("primary press alpha: %f\n", primaryPress->alpha);
        if (primaryPress->phase == Press::Phase::Immediate)
        {                                       
            printf("phase is immediate\n");
        }   
        else if (primaryPress->phase == Press::Phase::Sustained)
        {                                            
            printf("phase is sustained\n");
        }                   
        else if (primaryPress->phase == Press::Phase::Releasing)
        {                                            
            printf("phase is releasing\n");
        }                   
        glowSvg->setAlpha(std::clamp(primaryPress->getAlphaWithVeloApplied(), 0.f, 1.f));
    }                       
    else                    
    {                       
        glowSvg->setAlpha(0.f);
    }     

    SvgComponent::paint(g);
    const float scale = getScale();

    auto drawOutline = [&](const std::optional<Press> &press,
                           juce::Colour baseColour, int inset,
                           float baseThickness, float baseCornerRadius,
                           bool addInwardGlow = true)
    {
        if (!press)
        {
            return;
        }

        const float alpha = std::clamp(press->getAlphaWithVeloApplied(), 0.f, 1.f);

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
            juce::Colour inner = baseColour.withAlpha(baseAlpha * 0.05f);
            juce::Colour outer = baseColour.withAlpha(baseAlpha * 0.5f);

            juce::ColourGradient grad(inner, bounds.getCentreX(),
                                      bounds.getCentreY(), outer,
                                      bounds.getCentreX(),
                                      bounds.getCentreY() - bounds.getHeight() * 1.2f,
                                      true);

            grad.addColour(0.6, baseColour.withAlpha(baseAlpha * 0.55f));
            grad.addColour(1.0, outer);

            g.setGradientFill(grad);
            g.fillRoundedRectangle(bounds, cornerRadius);
        }

        g.setColour(baseColour.withAlpha(baseAlpha));
        g.drawRoundedRectangle(bounds, cornerRadius, thickness);
    };

    static const auto secondaryColor = juce::Colour::fromFloatRGBA(0.95f, 0.65f, 0.25f, 1.f);
    static const auto tertiaryColor = juce::Colour::fromFloatRGBA(0.3f, 0.5f, 0.95f, 0.6f);

    drawOutline(secondaryPress, secondaryColor, 3, 3.0f, 1.0f, true);
    drawOutline(tertiaryPress, tertiaryColor, 3, 3.0f, 1.0f, true);
}


