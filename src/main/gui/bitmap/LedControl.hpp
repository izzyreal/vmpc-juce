#pragma once
#include "Led.hpp"
#include "juce_graphics/juce_graphics.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include <Observer.hpp>

namespace mpc { class Mpc; }

class LedControl
: public juce::Timer
    , public mpc::Observer
{
    
private:
    mpc::Mpc& mpc;
    juce::Image ledGreen;
    juce::Image ledRed;
    
    juce::Rectangle<float> fullLevel{};
    juce::Rectangle<float> sixteenLevels{};
    juce::Rectangle<float> nextSeq{};
    juce::Rectangle<float> trackMute{};
    juce::Rectangle<float> padBankA{};
    juce::Rectangle<float> padBankB{};
    juce::Rectangle<float> padBankC{};
    juce::Rectangle<float> padBankD{};
    juce::Rectangle<float> after{};
    juce::Rectangle<float> undoSeq{};
    juce::Rectangle<float> rec{};
    juce::Rectangle<float> overDub{};
    juce::Rectangle<float> play{};
    
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
    void addAndMakeVisible(juce::Component* parent);
    void setTransform(juce::AffineTransform transform);
    void setBounds();
    
    void update(mpc::Observable* o, mpc::Message message) override;
    
    void timerCallback() override;
    
    LedControl(mpc::Mpc&, juce::Image& ledGreen, juce::Image& ledRed);
    ~LedControl() override;
    
};
