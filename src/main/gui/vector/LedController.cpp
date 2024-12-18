#include "LedController.hpp"

#include "Led.hpp"

#include "Mpc.hpp"
#include "sequencer/Sequencer.hpp"
#include "hardware/Hardware.hpp"
#include "hardware/Led.hpp"
#include "hardware/PadAndButtonKeyboard.hpp"
#include "controls/Controls.hpp"

using namespace vmpc_juce::gui::vector;

LedController::LedController(
        mpc::Mpc &mpcToUse,
        Led* fullLevelLedToUse,
        Led* sixteenLevelsLedToUse,
        Led* nextSeqLedToUse,
        Led* trackMuteLedToUse,
        Led* padBankALedToUse,
        Led* padBankBLedToUse,
        Led* padBankCLedToUse,
        Led* padBankDLedToUse,
        Led* afterLedToUse,
        Led* undoSeqLedToUse,
        Led* recLedToUse,
        Led* overDubLedToUse,
        Led* playLedToUse)
    : mpc(mpcToUse), fullLevelLed(fullLevelLedToUse), sixteenLevelsLed(sixteenLevelsLedToUse), nextSeqLed(nextSeqLedToUse), trackMuteLed(trackMuteLedToUse), padBankALed(padBankALedToUse), padBankBLed(padBankBLedToUse), padBankCLed(padBankCLedToUse), padBankDLed(padBankDLedToUse), afterLed(afterLedToUse), undoSeqLed(undoSeqLedToUse), recLed(recLedToUse), overDubLed(overDubLedToUse), playLed(playLedToUse)
{
    assert(fullLevelLed != nullptr);
    assert(sixteenLevelsLed != nullptr);
    assert(nextSeqLed != nullptr);
    assert(trackMuteLed != nullptr);
    assert(padBankALed != nullptr);
    assert(padBankBLed != nullptr);
    assert(padBankCLed != nullptr);
    assert(padBankDLed != nullptr);
    assert(afterLed != nullptr);
    assert(undoSeqLed != nullptr);
    assert(recLed != nullptr);
    assert(overDubLed != nullptr);
    assert(playLed != nullptr);

    for (auto &l : mpc.getHardware()->getLeds())
    {
        l->addObserver(this);
    }

    startTimer(25);
}

void LedController::setPadBankA(bool b)
{
    padBankALed->setLedOnEnabled(b);
}

void LedController::setPadBankB(bool b)
{
    padBankBLed->setLedOnEnabled(b);
}

void LedController::setPadBankC(bool b)
{
    padBankCLed->setLedOnEnabled(b);
}

void LedController::setPadBankD(bool b)
{
    padBankDLed->setLedOnEnabled(b);
}

void LedController::setFullLevel(bool b)
{
    fullLevelLed->setLedOnEnabled(b);
}

void LedController::setSixteenLevels(bool b)
{
    sixteenLevelsLed->setLedOnEnabled(b);
}

void LedController::setNextSeq(bool b)
{
    nextSeqLed->setLedOnEnabled(b);
}

void LedController::setTrackMute(bool b)
{
    trackMuteLed->setLedOnEnabled(b);
}

void LedController::setAfter(bool b)
{
    afterLed->setLedOnEnabled(b);
}

void LedController::setRec(bool b)
{
    recLed->setLedOnEnabled(b);
}

void LedController::setOverDub(bool b)
{
    overDubLed->setLedOnEnabled(b);
}

void LedController::setPlay(bool b)
{
    playLed->setLedOnEnabled(b);
}

void LedController::setUndoSeq(bool b)
{
    undoSeqLed->setLedOnEnabled(b);
}

void LedController::timerCallback()
{
    const auto screenName = mpc.getLayeredScreen()->getCurrentScreenName();

    if (screenName == "name")
    {
        const auto isUpperCase = mpc.getHardware()->getPadAndButtonKeyboard()->isUpperCase();
        setFullLevel(!isUpperCase);
        return;
    }

    const auto seq = mpc.getSequencer();
    const auto controls = mpc.getControls();
    const auto stepEditor = screenName == "step-editor";

    setUndoSeq(seq->isUndoSeqAvailable());

    setPlay(seq->isPlaying());

    auto isPlayingButNotOverdubbingAndOverdubIsPressed = seq->isPlaying() && !seq->isOverDubbing() && controls->isOverDubPressed();

    if (isPlayingButNotOverdubbingAndOverdubIsPressed)
    {
        setOverDub(false);
    }
    else
    {
        setOverDub(controls->isOverDubPressed() || seq->isOverDubbing() || stepEditor);
    }

    auto isPlayingButNotRecordingAndRecIsPressed = seq->isPlaying() && !seq->isRecording() && controls->isRecPressed();

    if (isPlayingButNotRecordingAndRecIsPressed)
    {
        setRec(false);
    }
    else
    {
        setRec(controls->isRecPressed() || seq->isRecording());
    }
}

void LedController::update(mpc::Observable*, mpc::Message message)
{
    const auto msg = std::get<std::string>(message);

    if (msg == "full-level-on") {
        setFullLevel(true);
    }
    else if(msg == "full-level-off") {
        setFullLevel(false);
    }
    else if (msg == "sixteen-levels-on") {
        setSixteenLevels(true);
    }
    else if (msg == "sixteen-levels-off") {
        setSixteenLevels(false);
    }
    else if (msg == "next-seq-on") {
        setNextSeq(true);
    }
    else if (msg == "next-seq-off") {
        setNextSeq(false);
    }
    else if (msg == "track-mute-on") {
        setTrackMute(true);
    }
    else if (msg == "track-mute-off") {
        setTrackMute(false);
    }
    else if (msg == "pad-bank-a-on") {
        setPadBankA(true);
    }
    else if (msg == "pad-bank-a-off") {
        setPadBankA(false);
    }
    else if (msg == "pad-bank-b-on") {
        setPadBankB(true);
    }
    else if (msg == "pad-bank-b-off") {
        setPadBankB(false);
    }
    else if (msg == "pad-bank-c-on") {
        setPadBankC(true);
    }
    else if (msg == "pad-bank-c-off") {
        setPadBankC(false);
    }
    else if (msg == "pad-bank-d-on") {
        setPadBankD(true);
    }
    else if (msg == "pad-bank-d-off") {
        setPadBankD(false);
    }
    else if (msg == "after-on") {
        setAfter(true);
    }
    else if (msg == "after-off") {
        setAfter(false);
    }
    else if (msg == "undo-seq-on") {
        setUndoSeq(true);
    }
    else if (msg == "undo-seq-off") {
        setUndoSeq(false);
    }
    else if (msg == "rec-on") {
        setRec(true);
    }
    else if (msg == "rec-off") {
        setRec(false);
    }
    else if (msg == "overdub-on") {
        setOverDub(true);
    }
    else if (msg == "overdub-off") {
        setOverDub(false);
    }
}

LedController::~LedController()
{
    for (auto &l : mpc.getHardware()->getLeds())
    {
        l->deleteObserver(this);
    }
}

