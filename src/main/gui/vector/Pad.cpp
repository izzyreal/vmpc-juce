#include "Pad.hpp"

#include "hardware/HardwareComponent.h"

#include <Mpc.hpp>
#include "file/AkaiName.hpp"
#include "juce_graphics/juce_graphics.h"
#include "lcdgui/screens/DrumScreen.hpp"

#include <sequencer/Track.hpp>

#include <sampler/Pad.hpp>
#include <sampler/NoteParameters.hpp>

#include <disk/SoundLoader.hpp>
#include <disk/MpcFile.hpp>

#include <lcdgui/screens/window/VmpcConvertAndLoadWavScreen.hpp>
#include <lcdgui/ScreenGroups.h>

#include <StrUtil.hpp>

#include <Logger.hpp>

#include <math.h>

using namespace vmpc_juce::gui::vector;
using namespace mpc::disk;
using namespace mpc::lcdgui::screens::window;
using namespace mpc::lcdgui::screens::dialog2;

static int getDrumIndexForCurrentScreen(mpc::Mpc& mpc, const std::string& currentScreenName)
{
    const bool isSamplerScreen = mpc::lcdgui::screengroups::isSamplerScreen(currentScreenName);
    return isSamplerScreen
        ? mpc.screens->get<mpc::lcdgui::screens::DrumScreen>()->getDrum()
        : mpc.getSequencer()->getActiveTrack()->getBus() - 1;
}

static std::shared_ptr<mpc::sampler::Program> getProgramForCurrentScreen(mpc::Mpc& mpc)
{
    const auto currentScreenName = mpc.getLayeredScreen()->getCurrentScreenName();
    const int drumIndex = getDrumIndexForCurrentScreen(mpc, currentScreenName);
    if (drumIndex < 0)
        return nullptr;

    auto sampler = mpc.getSampler();
    return sampler->getProgram(mpc.getDrum(drumIndex).getProgram());
}

Pad::Pad(juce::Component *commonParentWithShadowToUse,
         const float shadowSizeToUse,
         const std::function<float()> &getScaleToUse,
         mpc::Mpc &mpcToUse,
         std::shared_ptr<mpc::hardware::Pad> mpcPadToUse)
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
                //fading = true;
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
    if (mpc::StrUtil::hasEnding(mpc::StrUtil::toLower(path.toStdString()), ".snd") ||
        mpc::StrUtil::hasEnding(mpc::StrUtil::toLower(path.toStdString()), ".wav")) {
        auto sampler = mpc.getSampler();

        SoundLoader soundLoader(mpc, false);
        soundLoader.setPreview(false);

        auto compatiblePath = mpc::StrUtil::replaceAll(path.toStdString(), '\\', std::string("\\"));

        auto file = std::make_shared<mpc::disk::MpcFile>(fs::path(compatiblePath));

        auto layeredScreen = mpc.getLayeredScreen();

        SoundLoaderResult result;
        auto sound = mpc.getSampler()->addSound();

        if (sound == nullptr)
        {
            return;
        }

        soundLoader.loadSound(file, result, sound, shouldBeConverted);

        if (!result.success) {
            sampler->deleteSound(sound);

            if (result.canBeConverted) {
                auto loadRoutine = [&, path, layeredScreen]() {
                    const bool shouldBeConverted2 = true;
                    loadFile(path, shouldBeConverted2);
                };

                auto convertAndLoadWavScreen = mpc.screens->get<VmpcConvertAndLoadWavScreen>();
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

        mpc.getLayeredScreen()->showPopupForMs("LOADING " + mpc::StrUtil::padRight(soundFileName, " ", 16) + ext, 300);

        auto drumIndex = mpc.getSequencer()->getActiveTrack()->getBus() - 1;

        if (drumIndex == -1)
        {
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

    for (auto &f: files)
    {
        loadFile(f, shouldBeConverted);
    }
}

void Pad::sharedTimerCallback()
{
    static float decay = 0.05f;
    static auto applyDecay = [&](float &f) { f -= decay; };
    static float decayThreshold = 0.f;
    const int padIndexWithBank = mpcPad->getIndex() + (mpc.getBank() * 16);

    if (mpcPad->isPressed())
    {
        bool hasPrimaryPress = false;

        for (auto &p : primaryPresses)
        {
            if (p.alpha == 1.f)
            {
                hasPrimaryPress = true;
                break;
            }
        }

        if (!hasPrimaryPress)
        {
            primaryPresses.push_back({padIndexWithBank, 1.f, true});
        }
    }
    else
    {
        for (auto it = primaryPresses.begin(); it != primaryPresses.end();)
        {
            if (it->alpha <= decayThreshold)
            {
                primaryPresses.erase(it);
            }
            else
            {
                //it->alpha -= decay;
                applyDecay(it->alpha);
                ++it;
            }
        }
    }

    float primaryPressAlpha = 0.f;
    for (auto &p : primaryPresses) primaryPressAlpha += p.alpha;
    glowSvg->setAlpha(std::clamp(primaryPressAlpha, 0.f, 1.f));

    if (const auto program = getProgramForCurrentScreen(mpc); program)
    {
        using PadPressSource = mpc::sampler::Program::PadPressSource;

        const auto pressCountWithinActiveBank = program->isPadPressedBySource(padIndexWithBank, PadPressSource::NON_PHYSICAL);
        
        if (pressCountWithinActiveBank > 0)
        {
            bool hasSecondaryPress = false;

            for (auto &p : secondaryPresses)
            {
                if (p.alpha == 1.f && p.padIndexWithBank == padIndexWithBank)
                {
                    hasSecondaryPress = true;
                    break;
                }
            }

            if (!hasSecondaryPress)
            {
                secondaryPresses.push_back({padIndexWithBank, 1.f, false});
            }
        }
        else
        {
            for (auto it = secondaryPresses.begin(); it != secondaryPresses.end();)
            {
                if (it->alpha <= decayThreshold)
                {
                    secondaryPresses.erase(it);
                    continue;
                }

                applyDecay(it->alpha);
                ++it;
            }
        }

        for (int i = mpcPad->getIndex(); i < 64; i += 16)
        {
            if (i == padIndexWithBank) continue;

            const int programPressCount = program->isPadPressedBySource(i, PadPressSource::NON_PHYSICAL);

            if (programPressCount > 0)
            {
                bool hasTertiaryPress = false;

                for (auto &p : tertiaryPresses)
                {
                    if (p.alpha == 1.f && p.padIndexWithBank == i)
                    {
                        hasTertiaryPress = true;
                        break;
                    }
                }

                if (!hasTertiaryPress)
                {
                    tertiaryPresses.push_back({i, 1.f, false});
                }
            }
            else
            {
                for (auto it = tertiaryPresses.begin(); it != tertiaryPresses.end();)
                {
                    if (it->alpha <= decayThreshold)
                    {
                        tertiaryPresses.erase(it);
                        continue;
                    }
                    applyDecay(it->alpha);
                    ++it;
                }
            }
        }
    }

    bool bankHasChanged = false;

    if (mpc.getBank() != lastBank)
    {
        bankHasChanged = true;
        lastBank = mpc.getBank();
    }
    
    if (bankHasChanged)
    {
        for (auto &p : secondaryPresses)
        {
            if (p.alpha == 1.f) p.alpha -= decay;
        }

        for (auto &p : tertiaryPresses)
        {
            if (p.alpha == 1.f) p.alpha -= decay;
        }
    }

    repaint();
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

void Pad::paint(juce::Graphics& g)
{
    SvgComponent::paint(g);

    float secondaryPressAlpha = 0.f;

    for (auto &p : secondaryPresses)
    {
        if (p.isPhysical) continue;
        secondaryPressAlpha += p.alpha;
    }

    secondaryPressAlpha = std::clamp(secondaryPressAlpha, 0.f, 1.f);

    auto colour = juce::Colours::blue.withAlpha(0.5f * secondaryPressAlpha);

    g.setColour(colour);
    g.fillRoundedRectangle(getLocalBounds().reduced(20, 20).toFloat(), 4.0f);

    float tertiaryPressAlpha = 0.f;

    for (auto &p : tertiaryPresses)
    {
        if (p.isPhysical) continue;
        tertiaryPressAlpha += p.alpha;
    }

    tertiaryPressAlpha = std::clamp(tertiaryPressAlpha, 0.f, 1.f);

    colour = juce::Colours::crimson.withAlpha(0.5f * tertiaryPressAlpha);

    g.setColour(colour);
    g.fillRoundedRectangle(getLocalBounds().reduced(10, 10).toFloat(), 4.0f);
}

