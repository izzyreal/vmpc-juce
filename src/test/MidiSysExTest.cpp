#include <catch2/catch_test_macros.hpp>
#include "JuceToMpcMidiEventConvertor.hpp"
#include "MpcToJuceMidiEventConvertor.hpp"

using namespace vmpc_juce;
using namespace mpc::client::event;

TEST_CASE("JUCE MIDI conversion preserves bounded SysEx and sample offsets",
          "[sysex]")
{
    unsigned rejected = 0;
    auto pool = std::make_shared<mpc::sequencer::SysExPool>(
        [&]
        {
            ++rejected;
        });
    for (const int size : {2, 128, 129, 267, 512, 513})
    {
        std::vector<uint8_t> bytes(size, 0x47);
        bytes.front() = 0xF0;
        bytes.back() = 0xF7;
        const juce::MidiMessage input(bytes.data(), size, 37);
        auto event = JuceToMpcMidiEventConvertor::convert(input, pool.get());
        juce::MidiBuffer output;
        MpcToJuceMidiEventConvertor::addToBuffer(event, output);
        if (size > 512)
        {
            CHECK(event.getMessageType() == ClientMidiEvent::INVALID);
            CHECK(output.isEmpty());
        }
        else
        {
            REQUIRE(event.getMessageType() ==
                    ClientMidiEvent::SYSTEM_EXCLUSIVE);
            REQUIRE(output.getNumEvents() == 1);
            const auto meta = *output.begin();
            CHECK(meta.samplePosition == 37);
            REQUIRE(meta.numBytes == size);
            CHECK(std::equal(bytes.begin(), bytes.end(), meta.data));
        }
    }
    CHECK(rejected == 1);
    CHECK(pool->used() == 0);
}

TEST_CASE("Unsupported JUCE MIDI messages are not converted into notes",
          "[sysex]")
{
    const uint8_t status = 0xFE;
    const auto event =
        JuceToMpcMidiEventConvertor::convert(juce::MidiMessage(&status, 1));
    CHECK(event.getMessageType() == ClientMidiEvent::INVALID);
}
