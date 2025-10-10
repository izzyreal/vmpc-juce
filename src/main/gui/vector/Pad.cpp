#include "Pad.hpp"

#include <hardware/Hardware.hpp>
#include "hardware2/HardwareComponent.h"

#include <Mpc.hpp>
#include "file/AkaiName.hpp"

#include <sequencer/Track.hpp>

#include <sampler/NoteParameters.hpp>

#include <disk/SoundLoader.hpp>
#include <disk/MpcFile.hpp>

#include <lcdgui/screens/window/VmpcConvertAndLoadWavScreen.hpp>
#include <lcdgui/screens/dialog2/PopupScreen.hpp>

#include <StrUtil.hpp>

#include <Logger.hpp>

#include <math.h>

using namespace vmpc_juce::gui::vector;
using namespace mpc::disk;
using namespace mpc::lcdgui::screens::window;
using namespace mpc::lcdgui::screens::dialog2;

Pad::Pad(juce::Component *commonParentWithShadowToUse,
         const float shadowSizeToUse,
         const std::function<float()> &getScaleToUse,
         mpc::Mpc &mpcToUse,
         std::shared_ptr<mpc::hardware2::Pad> mpcPadToUse)
    : SvgComponent({"pad.svg", "pressed_pad.svg"}, commonParentWithShadowToUse, shadowSizeToUse, getScaleToUse), mpc(mpcToUse), mpcPad(mpcPadToUse)
{
    glowSvg = new SvgComponent({"pad_glow.svg"}, commonParentWithShadowToUse, 0.f, getScaleToUse);
    glowSvg->setAlpha(0.f);
    addAndMakeVisible(glowSvg);
}

bool Pad::isInterestedInFileDrag(const juce::StringArray &files)
{
    if (files.size() != 1)
    {
        return false;
    }

    for (auto &s: files)
    {
        if (mpc::StrUtil::hasEnding(mpc::StrUtil::toLower(s.toStdString()), ".snd") ||
            mpc::StrUtil::hasEnding(mpc::StrUtil::toLower(s.toStdString()), ".wav"))
        {
            if (glowSvg->getAlpha() == 0.f)
            {
                fading = true;
                glowSvg->setAlpha(1.f);
                repaint();
            }
            return true;
        }
    }
    return false;
}

void Pad::loadFile(const juce::String path, bool shouldBeConverted, std::string screenToReturnTo)
{
    if (mpc::StrUtil::hasEnding(mpc::StrUtil::toLower(path.toStdString()), ".snd") ||
        mpc::StrUtil::hasEnding(mpc::StrUtil::toLower(path.toStdString()), ".wav")) {
        auto sampler = mpc.getSampler();

        SoundLoader soundLoader(mpc, false);
        soundLoader.setPreview(false);

        auto compatiblePath = mpc::StrUtil::replaceAll(path.toStdString(), '\\', std::string("\\"));

        auto file = std::make_shared<mpc::disk::MpcFile>(fs::path(compatiblePath));

        auto layeredScreen = mpc.getLayeredScreen();

        SoundLoaderResult result;
        auto sound = mpc.getSampler()->addSound(screenToReturnTo);

        if (sound == nullptr)
        {
            return;
        }

        soundLoader.loadSound(file, result, sound, shouldBeConverted);
        auto popupScreen = mpc.screens->get<PopupScreen>("popup");

        if (!result.success) {
            sampler->deleteSound(sound);

            if (result.canBeConverted) {
                auto loadRoutine = [&, path, screenToReturnTo, layeredScreen]() {
                    const bool shouldBeConverted2 = true;
                    loadFile(path, shouldBeConverted2, screenToReturnTo);
                };

                auto convertAndLoadWavScreen = mpc.screens->get<VmpcConvertAndLoadWavScreen>(
                        "vmpc-convert-and-load-wav");
                convertAndLoadWavScreen->setLoadRoutine(loadRoutine);
                layeredScreen->openScreen("vmpc-convert-and-load-wav");
            }
            return;
        }

        std::string soundFileName;

        for (auto& c : mpc::StrUtil::toUpper(file->getNameWithoutExtension()))
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

        if (soundFileName.length() >= 16) {
            soundFileName = soundFileName.substr(0, 16);
        }

        // TODO Pad::loadFile should use the exact same logic as LoadScreen does when loading SND or WAV
        //  files. For now we preemptively bump the sound counter to make sure we don't get duplicate
        //  sound names. But if we follow LoadScreen logic, we will get a chance to CANCEL or RENAME in case
        //  a sound already exists. With the current logic, loading a file named "FULL1.SND" into an empty
        //  VMPC2000XL results in a sound named FULL2.

        soundFileName = sampler->addOrIncreaseNumber(soundFileName);
        sound->setName(soundFileName);

        auto ext = file->getExtension();

        popupScreen->setText("LOADING " + mpc::StrUtil::padRight(soundFileName, " ", 16) + ext);

        layeredScreen->openScreen("popup");
        popupScreen->returnToScreenAfterMilliSeconds(screenToReturnTo, 300);

        auto drumIndex = mpc.getSequencer()->getActiveTrack()->getBus() - 1;

        if (drumIndex == -1) {
            layeredScreen->openScreen(screenToReturnTo);
            return;
        }

        auto& drum = mpc.getDrum(drumIndex);

        auto programIndex = drum.getProgram();
        auto program = mpc.getSampler()->getProgram(programIndex);
        auto soundIndex = mpc.getSampler()->getSoundCount() - 1;
        auto padIndex = mpcPad->getIndex() + (mpc.getBank() * 16);
        auto programPad = program->getPad(padIndex);
        auto padNote = programPad->getNote();

        auto noteParameters = dynamic_cast<mpc::sampler::NoteParameters *>(program->getNoteParameters(padNote));

        if (noteParameters != nullptr)
        {
            noteParameters->setSoundIndex(soundIndex);
        }
    }
}

void Pad::filesDropped(const juce::StringArray &files, int, int)
{
    if (files.size() != 1) return;

    const bool shouldBeConverted = false;
    std::string screenToReturnTo = mpc.getLayeredScreen()->getCurrentScreenName();

    for (auto &f: files)
    {
        loadFile(f, shouldBeConverted, screenToReturnTo);
    }
}

void Pad::sharedTimerCallback()
{
    if (mpcPad->getVelocity() != lastProcessedVelo)
    {
        lastProcessedVelo = mpcPad->getVelocity();

        if (lastProcessedVelo.has_value())
        {
            glowSvg->setAlpha(static_cast<float>(*lastProcessedVelo) / 127.f);
            setSvgPath("pressed_pad.svg");
            fading = false;
            timerDivisionCounter = 0;
            repaint();
        }
        else
        {
            setSvgPath("pad.svg");
            fading = true;
        }
    }

    if (!fading || (timerDivisionCounter++ != 10))
    {
        return;
    }

    timerDivisionCounter = 0;

    glowSvg->setAlpha(glowSvg->getAlpha() - 0.08f);
    repaint();

    if (glowSvg->getAlpha() == 0.f)
    {
        fading = false;
    }
}

int Pad::getVelo(int veloY)
{
    return (float) veloY / getHeight();
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

