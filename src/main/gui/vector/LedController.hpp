#pragma once


#include <juce_events/juce_events.h>

#include <Observer.hpp>

namespace mpc { class Mpc; }

namespace vmpc_juce::gui::vector {

    class Led;

    class LedController : public juce::Timer, public mpc::Observer
    {

        private:
            mpc::Mpc &mpc;

            Led *fullLevelLed;
            Led *sixteenLevelsLed;
            Led *nextSeqLed;
            Led *trackMuteLed;
            Led *padBankALed;
            Led *padBankBLed;
            Led *padBankCLed;
            Led *padBankDLed;
            Led *afterLed;
            Led *undoSeqLed;
            Led *recLed;
            Led *overDubLed;
            Led *playLed;

        public:
            void setPadBankA(bool b);

        private:
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
            void update(mpc::Observable* o, mpc::Message message) override;

            void timerCallback() override;

            LedController(
                    mpc::Mpc &,
                    Led *fullLevelLed,
                    Led *sixteenLevelsLed,
                    Led *nextSeqLed,
                    Led *trackMuteLed,
                    Led *padBankALed,
                    Led *padBankBLed,
                    Led *padBankCLed,
                    Led *padBankDLed,
                    Led *afterLed,
                    Led *undoSeqLed,
                    Led *recLed,
                    Led *overDubLed,
                    Led *playLed);

            ~LedController() override;

    };
} // namespace vmpc_juce::gui::bitmap
