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
#pragma once
#include "Led.hpp"
#include "juce_graphics/juce_graphics.h"

#include <Observer.hpp>

namespace mpc { class Mpc; }

namespace vmpc_juce::gui::bitmap {
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
} // namespace vmpc_juce::gui::bitmap
