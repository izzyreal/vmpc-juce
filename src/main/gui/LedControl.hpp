#pragma once
#include "Led.hpp"
#include "juce_graphics/juce_graphics.h"
#include "juce_audio_processors/juce_audio_processors.h"

using namespace juce;

#include <observer/Observer.hpp>

namespace mpc { class Mpc; }

class LedControl
: public Timer
, public moduru::observer::Observer
{
    
private:
    mpc::Mpc& mpc;
    Image ledGreen;
    Image ledRed;
    
    Rectangle<float> fullLevel{};
    Rectangle<float> sixteenLevels{};
    Rectangle<float> nextSeq{};
    Rectangle<float> trackMute{};
    Rectangle<float> padBankA{};
    Rectangle<float> padBankB{};
    Rectangle<float> padBankC{};
    Rectangle<float> padBankD{};
    Rectangle<float> after{};
    Rectangle<float> undoSeq{};
    Rectangle<float> rec{};
    Rectangle<float> overDub{};
    Rectangle<float> play{};
    
    Led* fullLevelLed;
    Led* sixteenLevelsLed;
    Led* nextSeqLed;
    Led* trackMuteLed;
    Led* padBankALed;
    Led* padBankBLed;
    Led* padBankCLed;
    Led* padBankDLed;
    Led* afterLed;
    Led* undoSeqLed;
    Led* recLed;
    Led* overDubLed;
    Led* playLed;
    
public:
    void setPadBankA(bool b);
    void setPadBankB(bool b);
    void setPadBankC(bool b);
    void setPadBankD(bool b);
    void setFullLevel(bool b);
    void setSixteenLevels(bool b);
    void setNextSeq(bool b);
    void setTrackMute(bool b);
    void setAfter(bool b);
    void setRec(bool b);
    void setOverDub(bool b);
    void setPlay(bool b);
    void setUndoSeq(bool b);
    
public:
    void addAndMakeVisible(AudioProcessorEditor* editor);
    void setTransform(AffineTransform transform);
    void setBounds();
    
    void update(moduru::observer::Observable* o, nonstd::any arg) override;
    
    void timerCallback() override;
    
    LedControl(mpc::Mpc&, Image& ledGreen, Image& ledRed);
    ~LedControl() override;
    
};
