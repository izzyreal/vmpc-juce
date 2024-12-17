/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
#include "Pad.hpp"

#include <hardware/Hardware.hpp>
#include <hardware/HwPad.hpp>

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

Pad::Pad(juce::Component *commonParentWithShadowToUse, const float shadowSizeToUse, const std::function<float()> &getScaleToUse, mpc::Mpc &mpcToUse, std::weak_ptr<mpc::hardware::HwPad> padToUse)
    : SvgComponent({"pad.svg", "pressed_pad.svg"}, commonParentWithShadowToUse, shadowSizeToUse, getScaleToUse), mpc(mpcToUse), pad(padToUse)
{
    pad.lock()->addObserver(this);
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
                startTimer(100);
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
        auto padIndex = pad.lock()->getIndex() + (mpc.getBank() * 16);
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

void Pad::timerCallback()
{
    /*
    if (isShowingKeyboardMapping())
    {
        VmpcTooltipComponent::timerCallback();
        return;
    }
    */

    if (fading)
    {
        glowSvg->setAlpha(glowSvg->getAlpha() - 0.08f);
    }

    if (glowSvg->getAlpha() == 0.f)
    {
        fading = false;
        stopTimer();
    }

    repaint();
}

void Pad::update(mpc::Observable *, mpc::Message message)
{
    const auto handleUpdate = [message, this] {

        int velocity = std::get<int>(message);

        if (velocity == 255)
        {
            setSvgPath("pad.svg");
            fading = true;
        }
        else
        {
            if (velocity > 127)
            {
                velocity = 127;
            }

            glowSvg->setAlpha(velocity / 127.f);
            fading = false;
            startTimer(100);
            setSvgPath("pressed_pad.svg");
        }
    };

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        handleUpdate();
    }
    else
    {
        juce::MessageManager::callAsync(handleUpdate);
    }
}

int Pad::getVelo(int veloY)
{
    return (float) veloY / getHeight();
}

void Pad::mouseDrag(const juce::MouseEvent &event)
{
    if (!pad.lock()->isPressed())
        return;

    auto newVelo = getVelo(event.y);

    pad.lock()->setPressure(static_cast<unsigned char>(newVelo));
}

void Pad::paint(juce::Graphics &g)
{
    SvgComponent::paint(g);

    //VmpcTooltipComponent::paintButton(g, true, true);
}

void Pad::resized()
{
    SvgComponent::resized();
    glowSvg->setBounds(0, 0, getWidth(), getHeight());
}

Pad::~Pad()
{
    pad.lock()->deleteObserver(this);
    delete glowSvg;
}
