#include "Pad.hpp"

#include "FloatUtil.hpp"

#include <Mpc.hpp>
#include <StrUtil.hpp>

#include <file/AkaiName.hpp>
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
    auto file = std::make_shared<MpcFile>(mpc_fs::path(compatiblePath));
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
            const juce::Component::SafePointer<Pad> safePad(this);
            auto loadRoutine = [safePad, path]
            {
                if (safePad != nullptr)
                {
                    constexpr bool shouldBeConverted2 = true;
                    safePad->loadFile(path, shouldBeConverted2);
                }
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

void Pad::registerPhysicalPress(const mpc::Velocity velocity)
{
    glowState.registerPhysicalPress(velocity);
    updatePressedPadSvg();
    mutatedSinceLastPaint = true;
    fadeFrameCounter = 0;
}

void Pad::registerPhysicalAftertouch(const mpc::Pressure pressure)
{
    mutatedSinceLastPaint =
        glowState.registerPhysicalAftertouch(pressure) || mutatedSinceLastPaint;
}

void Pad::registerPhysicalRelease()
{
    if (glowState.registerPhysicalRelease())
    {
        updatePressedPadSvg();
        mutatedSinceLastPaint = true;
        fadeFrameCounter = 0;
    }
}

void Pad::registerProgramPress(const mpc::ProgramPadIndex index,
                               const mpc::Velocity velocity)
{
    glowState.registerProgramPress(index, velocity);
    mutatedSinceLastPaint = true;
    fadeFrameCounter = 0;
}

void Pad::registerProgramAftertouch(const mpc::ProgramPadIndex index,
                                    const mpc::Pressure pressure)
{
    mutatedSinceLastPaint =
        glowState.registerProgramAftertouch(index, pressure) ||
        mutatedSinceLastPaint;
}

void Pad::registerProgramRelease(const mpc::ProgramPadIndex index)
{
    if (glowState.registerProgramRelease(index))
    {
        mutatedSinceLastPaint = true;
        fadeFrameCounter = 0;
    }
}

void Pad::setActiveBank(const mpc::controller::Bank bank)
{
    if (glowState.setActiveBank(bank))
    {
        repaint();
    }
}

void Pad::updatePressedPadSvg()
{
    const bool shouldShowPressedPadSvg = glowState.isPhysicallyPressed();

    if (pressedPadSvgVisible == shouldShowPressedPadSvg)
    {
        return;
    }

    pressedPadSvgVisible = shouldShowPressedPadSvg;
    setSvgPath(pressedPadSvgVisible ? "pressed_pad.svg" : "pad.svg");
    mutatedSinceLastPaint = true;
}

void Pad::padTimerCallback()
{
    const float oldAlpha = glowSvg->getAlpha();
    mutatedSinceLastPaint = glowState.advance() || mutatedSinceLastPaint;
    updatePressedPadSvg();

    const float newAlpha = glowState.getLayers().physical;
    glowState.markPhysicalGlowDisplayed();

    if (!nearlyEqual(oldAlpha, newAlpha))
    {
        glowSvg->setAlpha(newAlpha);
        mutatedSinceLastPaint = true;
    }

    if (!mutatedSinceLastPaint)
    {
        return;
    }

    if (++fadeFrameCounter >= fadeRepaintInterval)
    {
        repaint();
        fadeFrameCounter = 0;
        mutatedSinceLastPaint = false;
    }
}

void Pad::paint(juce::Graphics &g)
{
    SvgComponent::paint(g);
    const float scale = getScale();

    auto drawOutline = [&](const float alpha, const juce::Colour baseColour,
                           const float inset, const float baseThickness,
                           const float baseCornerRadius,
                           const bool addInwardGlow = true)
    {
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

    static const auto selectedBankColor =
        juce::Colour::fromFloatRGBA(0.95f, 0.65f, 0.25f, 1.f);
    static const auto otherBanksColor =
        juce::Colour::fromFloatRGBA(0.35f, 0.55f, 1.f, 1.f).brighter();

    const auto layers = glowState.getLayers();
    drawOutline(layers.selectedBank, selectedBankColor, 2.65f, 1.5f, .4f, true);
    drawOutline(layers.otherBanks, otherBanksColor, 2.65f, 1.5f, .4f, true);
    glowState.markProgramGlowsDisplayed();
}
