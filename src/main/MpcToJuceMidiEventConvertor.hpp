#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <client/event/ClientMidiEvent.hpp>

namespace vmpc_juce
{
    class MpcToJuceMidiEventConvertor
    {
    public:
        using JuceMidiMessageAndSampleNumber =
            std::pair<juce::MidiMessage, int>;

        static std::optional<JuceMidiMessageAndSampleNumber>
        convert(const mpc::client::event::ClientMidiEvent &e)
        {
            using MpcEvent = mpc::client::event::ClientMidiEvent;

            juce::MidiMessage juceMsg;
            bool compatibleMsg = false;

            const auto channel = e.getChannel() + 1;
            const auto mpcType = e.getMessageType();

            if (mpcType == MpcEvent::NOTE_ON || mpcType == MpcEvent::NOTE_OFF)
            {
                const auto velocity = static_cast<juce::uint8>(e.getVelocity());

                juceMsg =
                    velocity == 0
                        ? juce::MidiMessage::noteOff(channel, e.getNoteNumber())
                        : juce::MidiMessage::noteOn(channel, e.getNoteNumber(),
                                                    velocity);
                compatibleMsg = true;
            }
            else if (mpcType == MpcEvent::MIDI_CLOCK)
            {
                juceMsg = juce::MidiMessage::midiClock();
                compatibleMsg = true;
            }
            else if (mpcType == MpcEvent::MIDI_START)
            {
                juceMsg = juce::MidiMessage::midiStart();
                compatibleMsg = true;
            }
            else if (mpcType == MpcEvent::MIDI_STOP)
            {
                juceMsg = juce::MidiMessage::midiStop();
                compatibleMsg = true;
            }
            else if (mpcType == MpcEvent::MIDI_CONTINUE)
            {
                juceMsg = juce::MidiMessage::midiContinue();
                compatibleMsg = true;
            }

            if (compatibleMsg)
            {
                return std::pair{juceMsg,
                                 static_cast<int>(e.getBufferOffset())};
            }

            return std::nullopt;
        }
    };
} // namespace vmpc_juce
