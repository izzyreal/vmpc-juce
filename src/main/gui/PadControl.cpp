#include "PadControl.hpp"
#include <hardware/Hardware.hpp>
#include <hardware/HwPad.hpp>

#include <Mpc.hpp>
#include "file/AkaiName.hpp"

#include <sequencer/Track.hpp>

#include <sampler/Pad.hpp>
#include <sampler/NoteParameters.hpp>

#include <disk/SoundLoader.hpp>
#include <disk/MpcFile.hpp>

#include <lcdgui/screens/window/VmpcConvertAndLoadWavScreen.hpp>
#include <lcdgui/screens/dialog2/PopupScreen.hpp>

#include <lang/StrUtil.hpp>

#include <Logger.hpp>

#include <math.h>

using namespace juce;
using namespace mpc::disk;
using namespace mpc::lcdgui::screens::window;
using namespace mpc::lcdgui::screens::dialog2;
using namespace moduru::lang;

PadControl::PadControl(mpc::Mpc &_mpc, juce::Rectangle<int> rectToUse, std::weak_ptr<mpc::hardware::HwPad> padToUse,
                       Image padHitImgToUse)
        : VmpcTooltipComponent(_mpc, padToUse.lock()), mpc(_mpc), pad(padToUse), padhitImg(padHitImgToUse), rect(rectToUse)
{
    pad.lock()->addObserver(this);
}

bool PadControl::isInterestedInFileDrag(const StringArray &files)
{
    if (files.size() != 1)
    {
        return false;
    }

    for (auto &s: files)
    {
        if (StrUtil::hasEnding(StrUtil::toLower(s.toStdString()), ".snd") ||
            StrUtil::hasEnding(StrUtil::toLower(s.toStdString()), ".wav"))
        {
            if (padhitBrightness == 0)
            {
                fading = true;
                padhitBrightness = 152;
                repaint();
                startTimer(100);
            }
            return true;
        }
    }
    return false;
}

void PadControl::loadFile(const String path, bool shouldBeConverted, std::string screenToReturnTo)
{
    if (StrUtil::hasEnding(StrUtil::toLower(path.toStdString()), ".snd") ||
        StrUtil::hasEnding(StrUtil::toLower(path.toStdString()), ".wav")) {
        auto sampler = mpc.getSampler();

        SoundLoader soundLoader(mpc, false);
        soundLoader.setPreview(false);

        auto compatiblePath = StrUtil::replaceAll(path.toStdString(), '\\', std::string("\\"));

        auto file = std::make_shared<mpc::disk::MpcFile>(fs::path(compatiblePath));

        auto layeredScreen = mpc.getLayeredScreen();

        SoundLoaderResult result;
        auto sound = mpc.getSampler()->addSound();

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

        for (auto& c : StrUtil::toUpper(file->getNameWithoutExtension()))
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

        // TODO PadControl::loadFile should use the exact same logic as LoadScreen does when loading SND or WAV
        //  files. For now we preemptively bump the sound counter to make sure we don't get duplicate
        //  sound names. But if we follow LoadScreen logic, we will get a chance to CANCEL or RENAME in case
        //  a sound already exists. With the current logic, loading a file named "FULL1.SND" into an empty
        //  VMPC2000XL results in a sound named FULL2.

        soundFileName = sampler->addOrIncreaseNumber(soundFileName);
        sound->setName(soundFileName);

        auto ext = file->getExtension();

        popupScreen->setText("LOADING " + StrUtil::padRight(soundFileName, " ", 16) + ext);

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

void PadControl::filesDropped(const StringArray &files, int, int)
{
    if (files.size() != 1) return;

    const bool shouldBeConverted = false;
    std::string screenToReturnTo = mpc.getLayeredScreen()->getCurrentScreenName();

    for (auto &f: files)
    {
        loadFile(f, shouldBeConverted, screenToReturnTo);
    }
}

void PadControl::timerCallback()
{
    if (fading)
    {
        padhitBrightness -= 20;
    }

    if (padhitBrightness < 0)
    {
        padhitBrightness = 0;
        repaint();
        fading = false;
        stopTimer();
    }
    else
    {
        repaint();
    }
}

void PadControl::update(moduru::observer::Observable *, nonstd::any arg)
{
    int velocity = nonstd::any_cast<int>(arg);

    if (velocity == 255)
    {
        fading = true;
    }
    else
    {
        padhitBrightness = velocity + 25;
        fading = false;
        startTimer(100);
    }
}

int PadControl::getVelo(int veloX, int veloY)
{
    auto centX = rect.getCentreX() - rect.getX();
    auto centY = rect.getCentreY() - rect.getY();
    auto distX = veloX - centX;
    auto distY = veloY - centY;
    auto powX = pow(distX, 2);
    auto powY = pow(distY, 2);
    auto dist = sqrt(powX + powY);

    if (dist > 46) dist = 46;
    int velo = static_cast<int>(127.0 - (dist * (127.0 / 48.0)));
    return velo;
}

void PadControl::mouseDown(const MouseEvent &event)
{
    pad.lock()->push(getVelo(event.x, event.y));
}

void PadControl::mouseDoubleClick(const MouseEvent &)
{
}

void PadControl::mouseUp(const MouseEvent &)
{
    pad.lock()->release();
}

void PadControl::mouseDrag(const MouseEvent &event)
{
    if (!pad.lock()->isPressed())
        return;

    auto newVelo = getVelo(event.x, event.y);

    pad.lock()->setPressure(static_cast<unsigned char>(newVelo));
}

void PadControl::setBounds()
{
    setSize(static_cast<int>(rect.getWidth()), static_cast<int>(rect.getHeight()));
    Component::setBounds(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void PadControl::paint(Graphics &g)
{
    auto img = padhitImg.createCopy();
    auto mult = padhitBrightness / 150.0;
    img.multiplyAllAlphas(static_cast<float>(mult));
    g.drawImageAt(img, 0, 0);
}

PadControl::~PadControl()
{
    pad.lock()->deleteObserver(this);
}
