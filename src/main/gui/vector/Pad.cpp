#include "Pad.hpp"

#include "FloatUtil.hpp"

#include <Mpc.hpp>
#include <StrUtil.hpp>

#include <file/AkaiName.hpp>
#include <utils/TimeUtils.hpp>
#include <controller/ClientEventController.hpp>
#include <performance/PerformanceManager.hpp>
#include <hardware/Component.hpp>

#include <sampler/Sampler.hpp>
#include <sampler/Pad.hpp>
#include <sampler/NoteParameters.hpp>

#include <sequencer/Sequencer.hpp>
#include <sequencer/Track.hpp>
#include <sequencer/Bus.hpp>

#include <disk/SoundLoader.hpp>
#include <disk/MpcFile.hpp>

#include <lcdgui/screens/window/VmpcConvertAndLoadWavScreen.hpp>

#include <juce_graphics/juce_graphics.h>

using namespace vmpc_juce::gui::vector;
using namespace mpc::disk;
using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens::window;
using namespace mpc::lcdgui::screens::dialog2;
using namespace mpc::sequencer;

Pad::Pad(Component *commonParentWithShadowToUse, const float shadowSizeToUse,
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
        if (const auto lower = mpc::StrUtil::toLower(s.toStdString());
            mpc::StrUtil::hasEnding(lower, ".snd") ||
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

void Pad::loadFile(const juce::String &path, bool shouldBeConverted)
{
    if (const auto lower = mpc::StrUtil::toLower(path.toStdString());
        !mpc::StrUtil::hasEnding(lower, ".snd") &&
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
            auto loadRoutine = [&, path]
            {
                constexpr bool shouldBeConverted2 = true;
                loadFile(path, shouldBeConverted2);
            };
            auto convertAndLoadWavScreen =
                mpc.screens->get<ScreenId::VmpcConvertAndLoadWavScreen>();
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
        mpc.getSequencer()->getSelectedTrack()->getBusType());
    if (!drumBus)
    {
        return;
    }

    auto programIndex = drumBus->getProgramIndex();
    auto program = mpc.getSampler()->getProgram(programIndex);
    auto soundIndex = mpc.getSampler()->getSoundCount() - 1;
    const auto bank = mpc.clientEventController->getActiveBank();
    auto programPadIndex = mpc::controller::physicalPadAndBankToProgramPadIndex(
        mpcPad->getIndex(), bank);
    auto programPad = program->getPad(programPadIndex);
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

    constexpr bool shouldBeConverted = false;
    loadFile(files[0], shouldBeConverted);
}

int Pad::getVelo(const int veloY) const
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

void Pad::padTimerCallback()
{
    static constexpr float decay = 0.003f;
    static constexpr float immediateDecay = 0.005f;
    static constexpr float immediateFadeFactorPrimary = 0.8f;
    static constexpr float immediateFadeFactorNonPrimary = 0.6f;
    static constexpr float decayThreshold = 0.f;

    bool hardMutated = false;
    bool softMutated = false;

    auto decayPress = [&](std::optional<Press> &press,
                          const bool isPrimary) -> bool
    {
        if (!press)
        {
            return false;
        }

        if (press->alpha == 1.f && !press->wasPaintedWithInitialAlpha)
        {
            return false;
        }

        bool faded = false;
        const float fadeFactor = isPrimary ? immediateFadeFactorPrimary
                                           : immediateFadeFactorNonPrimary;

        switch (press->phase)
        {
            case Press::Phase::Immediate:
                press->alpha -= immediateDecay;
                if (press->alpha <= fadeFactor)
                {
                    press->alpha = fadeFactor;
                    press->phase = Press::Phase::Sustained;
                }
                faded = true;
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
                faded = true;
                break;
        }

        return faded;
    };

    const auto bank = mpc.clientEventController->getActiveBank();
    const auto programPadIndex =
        mpc::controller::physicalPadAndBankToProgramPadIndex(mpcPad->getIndex(),
                                                             bank);

    if (mpcPad->isPressed())
    {
        const auto v =
            mpcPad->getPressure().value_or(mpcPad->getVelocity().value());

        if (!primaryPress || primaryPress->phase == Press::Phase::Releasing)
        {
            primaryPress =
                Press{programPadIndex, 1.f, v, Press::Phase::Immediate,
                      mpc::utils::nowInMilliseconds()};

            hardMutated = true;
        }
        else
        {
            if (primaryPress->veloOrPressure != v)
            {
                primaryPress->veloOrPressure = v;
                hardMutated = true;
            }

            if (decayPress(primaryPress, true))
            {
                softMutated = true;
            }
        }
    }
    else if (primaryPress)
    {
        if (primaryPress->phase != Press::Phase::Releasing)
        {
            primaryPress->phase = Press::Phase::Releasing;
            hardMutated = true;
        }

        if (decayPress(primaryPress, true))
        {
            softMutated = true;
        }
    }

    if (static_cast<int>(bank) != lastBank)
    {
        lastBank = static_cast<int>(bank);

        auto oldSecondary = secondaryPress;
        auto oldTertiary = tertiaryPress;

        secondaryPress.reset();
        tertiaryPress.reset();

        const auto physicalPad = mpcPad->getIndex();
        constexpr auto padsPerBank = mpc::Mpc2000XlSpecs::PADS_PER_BANK_COUNT;

        const int expectedSecondaryIndex =
            mpc::controller::physicalPadAndBankToProgramPadIndex(physicalPad,
                                                                 bank);

        auto reassign = [&](const std::optional<Press> &oldPress)
        {
            if (!oldPress)
            {
                return;
            }

            const int idx = oldPress->padIndexWithBank;

            if (idx == expectedSecondaryIndex)
            {
                secondaryPress = oldPress;
                hardMutated = true;
            }
            else if (idx % padsPerBank == physicalPad)
            {
                tertiaryPress = oldPress;
                hardMutated = true;
            }
        };

        reassign(oldSecondary);
        reassign(oldTertiary);
    }

    const auto snapshot = mpc.getPerformanceManager().lock()->getSnapshot();
    static const std::vector exclude{
        mpc::performance::PerformanceEventSource::VirtualMpcHardware};

    const auto recent = snapshot.getMostRecentProgramPadPress(
        mpc::ProgramPadIndex(programPadIndex), exclude);

    if (recent)
    {
        const auto v = snapshot.getPressedProgramPadAfterTouchOrVelocity(
            mpc::ProgramPadIndex(programPadIndex));

        if (!secondaryPress ||
            secondaryPress->phase == Press::Phase::Releasing ||
            secondaryPress->pressTime != recent->pressTimeMs)
        {
            secondaryPress =
                Press{programPadIndex, 1.f, v, Press::Phase::Immediate,
                      recent->pressTimeMs};
            hardMutated = true;
        }
        else
        {
            if (secondaryPress->veloOrPressure != v)
            {
                secondaryPress->veloOrPressure = v;
                hardMutated = true;
            }

            if (decayPress(secondaryPress, false))
            {
                softMutated = true;
            }
        }
    }
    else if (secondaryPress)
    {
        if (secondaryPress->phase != Press::Phase::Releasing)
        {
            secondaryPress->phase = Press::Phase::Releasing;
            hardMutated = true;
        }

        if (decayPress(secondaryPress, false))
        {
            softMutated = true;
        }
    }

    int8_t otherBanked = -1;
    std::optional<mpc::performance::ProgramPadPressEvent> otherPress;

    for (int8_t i = mpcPad->getIndex(); i < mpc::MaxProgramPadIndex;
         i += mpc::Mpc2000XlSpecs::PADS_PER_BANK_COUNT)
    {
        if (i == programPadIndex)
        {
            continue;
        }

        if (auto p = snapshot.getMostRecentProgramPadPress(
                mpc::ProgramPadIndex(i), exclude))
        {
            otherBanked = i;
            otherPress = p;
            break;
        }
    }

    if (otherBanked != -1 && otherPress)
    {
        const auto v = snapshot.getPressedProgramPadAfterTouchOrVelocity(
            mpc::ProgramPadIndex(otherBanked));

        if (!tertiaryPress || tertiaryPress->phase == Press::Phase::Releasing ||
            tertiaryPress->pressTime != otherPress->pressTimeMs)
        {
            tertiaryPress = Press{otherBanked, 1.f, v, Press::Phase::Immediate,
                                  otherPress->pressTimeMs};
            hardMutated = true;
        }
        else
        {
            if (tertiaryPress->veloOrPressure != v)
            {
                tertiaryPress->veloOrPressure = v;
                hardMutated = true;
            }

            if (decayPress(tertiaryPress, false))
            {
                softMutated = true;
            }
        }
    }
    else if (tertiaryPress)
    {
        if (tertiaryPress->phase != Press::Phase::Releasing)
        {
            tertiaryPress->phase = Press::Phase::Releasing;
            hardMutated = true;
        }

        if (decayPress(tertiaryPress, false))
        {
            softMutated = true;
        }
    }

    float newPrimaryAlpha = 0.f;

    if (primaryPress)
    {
        newPrimaryAlpha =
            std::clamp(primaryPress->getAlphaWithVeloApplied(), 0.f, 1.f);

        if (primaryPress->alpha == 1.f &&
            !primaryPress->wasPaintedWithInitialAlpha)
        {
            primaryPress->wasPaintedWithInitialAlpha = true;
        }
    }

    const bool alphaChanged =
        !nearlyEqual(glowSvg->getAlpha(), newPrimaryAlpha);

    if (hardMutated)
    {
        if (alphaChanged)
        {
            glowSvg->setAlpha(newPrimaryAlpha);
        }

        mutatedSinceLastPaint = false;
        fadeFrameCounter = 0;

        if (!alphaChanged)
        {
            repaint();
        }

        return;
    }

    if (softMutated || alphaChanged)
    {
        mutatedSinceLastPaint = true;
    }

    if (mutatedSinceLastPaint)
    {
        if (++fadeFrameCounter >= fadeRepaintInterval)
        {
            if (alphaChanged)
            {
                glowSvg->setAlpha(newPrimaryAlpha);
            }

            repaint();
            fadeFrameCounter = 0;
            mutatedSinceLastPaint = false;
        }
    }
}

void Pad::paint(juce::Graphics &g)
{
    SvgComponent::paint(g);
    const float scale = getScale();

    auto drawOutline =
        [&](std::optional<Press> &press, const juce::Colour baseColour,
            const float inset, const float baseThickness,
            const float baseCornerRadius, const bool addInwardGlow = true)
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
            const juce::Colour inner = baseColour.withAlpha(baseAlpha * 0.05f);
            const juce::Colour outer = baseColour.withAlpha(baseAlpha * 0.5f);

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
