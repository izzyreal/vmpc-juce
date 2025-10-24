#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "input/HostInputEvent.hpp"

namespace vmpc_juce {
    class JuceToMpcMidiEventConvertor {
        public:
            static mpc::input::MidiEvent convert(const juce::MidiMessage &m)
            {
                mpc::input::MidiEvent mpcMidiEvent;
                mpcMidiEvent.setChannel(m.getChannel() - 1);
                mpcMidiEvent.setBufferOffset(m.getTimeStamp());

                if (m.isNoteOn())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::NOTE_ON);
                    mpcMidiEvent.setNoteNumber(m.getNoteNumber());
                    mpcMidiEvent.setVelocity(m.getVelocity());
                }
                else if (m.isNoteOff())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::NOTE_OFF);
                    mpcMidiEvent.setNoteNumber(m.getNoteNumber());
                    mpcMidiEvent.setVelocity(m.getVelocity());
                }
                else if (m.isController())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::CONTROLLER);
                    mpcMidiEvent.setControllerNumber(m.getControllerNumber());
                    mpcMidiEvent.setControllerValue(m.getControllerValue());
                }
                else if (m.isAftertouch())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::AFTERTOUCH);
                    mpcMidiEvent.setAftertouchNote(m.getNoteNumber());
                    mpcMidiEvent.setAftertouchValue(m.getAfterTouchValue());
                }
                else if (m.isChannelPressure())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::CHANNEL_PRESSURE);
                    mpcMidiEvent.setChannelPressure(m.getChannelPressureValue());
                }
                else if (m.isMidiClock())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::MIDI_CLOCK);
                }
                else if (m.isMidiStart())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::MIDI_START);
                }
                else if (m.isMidiContinue())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::MIDI_CONTINUE);
                }
                else if (m.isMidiStop())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::MIDI_STOP);
                }
                else if (m.isProgramChange())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::PROGRAM_CHANGE);
                    mpcMidiEvent.setProgramNumber(m.getProgramChangeNumber());
                }
                else if (m.isPitchWheel())
                {
                    mpcMidiEvent.setMessageType(mpc::input::MidiEvent::PITCH_WHEEL);
                    mpcMidiEvent.setPitchWheelValue(m.getPitchWheelValue());
                }

                return mpcMidiEvent;
            }
    };
} // namespace vmpc_juce
