#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <client/event/ClientMidiEvent.hpp>

namespace vmpc_juce
{
    class JuceToMpcMidiEventConvertor
    {
    public:
        static mpc::client::event::ClientMidiEvent
        convert(const juce::MidiMessage &m)
        {
            mpc::client::event::ClientMidiEvent mpcMidiEvent;
            mpcMidiEvent.setChannel(m.getChannel() - 1);
            mpcMidiEvent.setBufferOffset(m.getTimeStamp());

            if (m.isNoteOn())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::NOTE_ON);
                mpcMidiEvent.setNoteNumber(m.getNoteNumber());
                mpcMidiEvent.setVelocity(m.getVelocity());
            }
            else if (m.isNoteOff())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::NOTE_OFF);
                mpcMidiEvent.setNoteNumber(m.getNoteNumber());
                mpcMidiEvent.setVelocity(m.getVelocity());
            }
            else if (m.isController())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::CONTROLLER);
                mpcMidiEvent.setControllerNumber(m.getControllerNumber());
                mpcMidiEvent.setControllerValue(m.getControllerValue());
            }
            else if (m.isAftertouch())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::AFTERTOUCH);
                mpcMidiEvent.setAftertouchNote(m.getNoteNumber());
                mpcMidiEvent.setAftertouchValue(m.getAfterTouchValue());
            }
            else if (m.isChannelPressure())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::CHANNEL_PRESSURE);
                mpcMidiEvent.setChannelPressure(m.getChannelPressureValue());
            }
            else if (m.isMidiClock())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::MIDI_CLOCK);
            }
            else if (m.isMidiStart())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::MIDI_START);
            }
            else if (m.isMidiContinue())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::MIDI_CONTINUE);
            }
            else if (m.isMidiStop())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::MIDI_STOP);
            }
            else if (m.isProgramChange())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::PROGRAM_CHANGE);
                mpcMidiEvent.setProgramNumber(m.getProgramChangeNumber());
            }
            else if (m.isPitchWheel())
            {
                mpcMidiEvent.setMessageType(
                    mpc::client::event::ClientMidiEvent::PITCH_WHEEL);
                mpcMidiEvent.setPitchWheelValue(m.getPitchWheelValue());
            }

            return mpcMidiEvent;
        }
    };
} // namespace vmpc_juce
