#include "VmpcProcessor.hpp"
#include "Allocations.hpp"
#include "audiomidi/EventHandler.hpp"
#include "engine/EngineHost.hpp"
#include "engine/IndivFxMixer.hpp"
#include "engine/Voice.hpp"
#include "lcdgui/screens/MixerSetupScreen.hpp"
#include "sampler/Sampler.hpp"
#include "sampler/Sound.hpp"
#include "sampler/Program.hpp"
#include "sampler/NoteParameters.hpp"
#include "sequencer/Sequencer.hpp"
#include "sequencer/Bus.hpp"
#include "sequencer/EventData.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/resource.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif
using json = nlohmann::json;
using Clock = std::chrono::steady_clock;
using namespace mpc;

static double cpuSeconds()
{
#if defined(_WIN32)
    FILETIME creation, exit, kernel, user;
    if (!GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user))
    {
        throw std::runtime_error("GetThreadTimes failed");
    }
    auto value = [](FILETIME t)
    {
        return (uint64_t(t.dwHighDateTime) << 32) | t.dwLowDateTime;
    };
    return (value(kernel) + value(user)) * 1e-7;
#else
    timespec t{};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t) != 0)
    {
        throw std::runtime_error("thread CPU clock failed");
    }
    return double(t.tv_sec) + t.tv_nsec * 1e-9;
#endif
}
static json memory()
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX m{};
    m.cb = sizeof(m);
    if (!GetProcessMemoryInfo(GetCurrentProcess(),
                              reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&m),
                              sizeof(m)))
    {
        throw std::runtime_error("GetProcessMemoryInfo failed");
    }
    return {{"resident_bytes", m.WorkingSetSize},
            {"peak_resident_bytes", m.PeakWorkingSetSize},
            {"private_bytes", m.PrivateUsage}};
#elif defined(__APPLE__)
    mach_task_basic_info_data_t m{};
    mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&m), &size) != KERN_SUCCESS)
    {
        throw std::runtime_error("task_info failed");
    }
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return {{"resident_bytes", m.resident_size},
            {"peak_resident_bytes", usage.ru_maxrss},
            {"private_bytes", nullptr}};
#else
    std::ifstream f("/proc/self/statm");
    uint64_t total, resident;
    if (!(f >> total >> resident))
    {
        throw std::runtime_error("Cannot read /proc/self/statm");
    }
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return {{"resident_bytes", resident * uint64_t(sysconf(_SC_PAGESIZE))},
            {"peak_resident_bytes", usage.ru_maxrss * 1024},
            {"private_bytes", nullptr}};
#endif
}
static void environment(const char *key, const std::string &value)
{
#if defined(_WIN32)
    _putenv_s(key, value.c_str());
#else
    setenv(key, value.c_str(), 1);
#endif
}
static int activeVoices(vmpc_juce::VmpcProcessor &processor)
{
    const auto &voices = processor.mpc.getEngineHost()->getVoices();
    return int(std::count_if(voices.begin(), voices.end(),
                             [](const auto &voice)
                             {
                                 return !voice->isFinished();
                             }));
}
int main(int argc, char **argv)
{
    try
    {
        if (argc != 3)
        {
            throw std::runtime_error("Usage: worker request.json result.json");
        }
        json request;
        std::ifstream(argv[1]) >> request;
        const int frames = request.at("frames"), rate = request.at("rate");
        const bool mono = request.at("mono"), drum = request.at("drum");
        const std::string routing = request.at("routing");
        const bool idle = request.value("idle", false),
                   tail = request.value("tail", false);
        const bool allocations = request.value("allocations", false);
        const double seconds = request.at("seconds"),
                     warmup = request.at("warmup");
        if (frames <= 0 || frames > 8192 || rate < 8000 || rate > 192000 ||
            seconds <= 0 || warmup < 0)
        {
            throw std::runtime_error("Invalid workload arguments");
        }
        const auto sandbox = juce::File::createTempFile("-vmpc-benchmark");
        struct Cleanup
        {
            juce::File path;
            ~Cleanup()
            {
                path.deleteRecursively();
            }
        } cleanup{sandbox};
        sandbox.createDirectory();
        environment(
            "VMPC2000XL_DOCUMENTS_PATH",
            sandbox.getChildFile("documents").getFullPathName().toStdString());
        environment(
            "VMPC2000XL_CONFIG_HOME",
            sandbox.getChildFile("config").getFullPathName().toStdString());
        juce::ScopedJuceInitialiser_GUI juceInit;
        juce::PluginHostType::jucePlugInClientCurrentWrapperType =
            juce::AudioProcessor::wrapperType_VST3;
        juce::ScopedNoDenormals noDenormals;
        struct PlayHead final : juce::AudioPlayHead
        {
            juce::Optional<PositionInfo> getPosition() const override
            {
                PositionInfo position;
                position.setBpm(120.0);
                position.setPpqPosition(0.0);
                position.setTimeInSamples(0);
                position.setIsPlaying(false);
                return position;
            }
        } playHead;
        vmpc_juce::VmpcProcessor processor;
        processor.setPlayHead(&playHead);
        if (!processor.hasRequiredResources())
        {
            throw std::runtime_error(
                processor.getRequiredResourcesFailureMessage());
        }
        auto layout = processor.getBusesLayout();
        for (int i = 0; i < layout.inputBuses.size(); ++i)
        {
            layout.inputBuses.set(i, i == 0
                                         ? juce::AudioChannelSet::stereo()
                                         : juce::AudioChannelSet::disabled());
        }
        for (int i = 0; i < layout.outputBuses.size(); ++i)
        {
            layout.outputBuses.set(i, (routing == "stereo" && i > 0)
                                          ? juce::AudioChannelSet::disabled()
                                      : i < 6 ? juce::AudioChannelSet::stereo()
                                              : juce::AudioChannelSet::mono());
        }
        if (!processor.setBusesLayout(layout))
        {
            throw std::runtime_error("Processor rejected benchmark layout");
        }
        const auto engine = processor.mpc.getEngineHost();
        engine->setPhysicalSoundsEnabled(false);
        processor.setRateAndBufferSizeDetails(rate, frames);
        processor.prepareToPlay(rate, frames);
        juce::AudioBuffer<float> buffer(
            std::max(processor.getTotalNumInputChannels(),
                     processor.getTotalNumOutputChannels()),
            frames);
        juce::MidiBuffer midi;
        midi.ensureSize(8192);
        buffer.clear();
        processor.processBlock(buffer, midi);
        const auto sampler = processor.mpc.getSampler();
        const int firstSound = sampler->getSoundCount();
        constexpr int sampleFrames = 44100;
        // Separate sample storage exercises the working set of 32 distinct
        // sounds, including on older CPUs with much smaller caches.
        for (int voice = 0; voice < 32; ++voice)
        {
            const auto sound = sampler->addSound();
            sound->setMono(mono);
            auto samples = sound->getMutableSampleData();
            samples->resize(sampleFrames * (mono ? 1 : 2));
            for (int i = 0; i < sampleFrames; ++i)
            {
                (*samples)[i] =
                    float(0.025 * std::sin(i * (0.031 + voice * 0.0001)) +
                          0.015 * std::sin(i * 0.073));
                if (!mono)
                {
                    (*samples)[sampleFrames + i] =
                        float(0.02 * std::sin(i * (0.047 + voice * 0.0001)));
                }
            }
            sound->setStart(0);
            sound->setEnd(sampleFrames);
            sound->setLoopTo(0);
            sound->setLoopEnabled(true);
        }
        const auto program = sampler->getProgram(0);
        const auto drumBus =
            processor.mpc.getSequencer()->getDrumBus(DrumBusIndex(0));
        drumBus->setProgramIndex(ProgramIndex(0));
        const auto mixerSetup =
            processor.mpc.screens->get<lcdgui::ScreenId::MixerSetupScreen>();
        mixerSetup->setStereoMixSourceDrum(drum);
        mixerSetup->setIndivFxSourceDrum(drum);
        for (int i = 0; i < 32; ++i)
        {
            const auto note =
                program->getNoteParameters(DrumNoteNumber(35 + i));
            note->setSoundIndex(firstSound + i);
            note->setTune(7 + i % 5);
            note->setFilterFrequency(65);
            note->setFilterResonance(20);
            note->setDecay(100);
            note->setDecayMode(0);
            const int output = routing == "spread" || tail ? i % 8 + 1 : 0;
            note->getIndivFxMixer()->setOutput(
                DrumMixerIndividualOutput(output));
            note->getIndivFxMixer()->setVolumeIndividualOut(
                DrumMixerLevel(100));
            drumBus->getIndivFxMixerChannels()[i]->setOutput(
                DrumMixerIndividualOutput(output));
            drumBus->getIndivFxMixerChannels()[i]->setVolumeIndividualOut(
                DrumMixerLevel(100));
        }
        engine->prepareProcessBlock(frames);
        if (!idle)
        {
            for (int i = 0; i < 32; ++i)
            {
                sequencer::EventData event;
                event.type = sequencer::EventType::NoteOn;
                event.noteNumber = NoteNumber(35 + i);
                event.velocity = Velocity(100);
                event.duration = NoDuration;
                processor.mpc.getSequencer()
                    ->getEventHandler()
                    ->handleUnfinalizedNoteOn(event, std::nullopt,
                                              sequencer::BusType::DRUM1);
            }
        }
        const int expectedVoices = idle ? 0 : 32;
        auto render = [&]
        {
            buffer.clear();
            midi.clear();
            processor.processBlock(buffer, midi);
        };
        auto validateVoices = [&]
        {
            if (activeVoices(processor) != expectedVoices)
            {
                throw std::runtime_error(
                    "Expected " + std::to_string(expectedVoices) +
                    " active voices, got " +
                    std::to_string(activeVoices(processor)));
            }
        };
        // Deterministic validation prefix, independent of measurement duration.
        double energy = 0;
        uint64_t hash = 14695981039346656037ULL;
        std::vector<uint64_t> channelHashes(buffer.getNumChannels(), hash);
        std::vector<double> channelEnergy(buffer.getNumChannels(), 0.0);
        for (int block = 0; block < 64; ++block)
        {
            render();
            validateVoices();
            for (int c = 0; c < buffer.getNumChannels(); ++c)
            {
                for (int f = 0; f < frames; ++f)
                {
                    const float sample = buffer.getSample(c, f);
                    if (!std::isfinite(sample))
                    {
                        throw std::runtime_error("Non-finite output");
                    }
                    energy += double(sample) * sample;
                    channelEnergy[c] += double(sample) * sample;
                    uint32_t bits;
                    std::memcpy(&bits, &sample, sizeof(bits));
                    hash ^= bits;
                    hash *= 1099511628211ULL;
                    channelHashes[c] ^= bits;
                    channelHashes[c] *= 1099511628211ULL;
                }
            }
        }
        if (!idle && energy <= 1e-12)
        {
            throw std::runtime_error("Silent 32-voice workload");
        }
        double tailEnergy = 0;
        std::vector<uint64_t> tailHashes(buffer.getNumChannels(),
                                         14695981039346656037ULL);
        std::vector<double> tailChannelEnergy(buffer.getNumChannels(), 0.0);
        if (tail || request.value("tail_reference", false))
        {
            if (tail)
            {
                for (int i = 0; i < 32; ++i)
                {
                    program->getNoteParameters(DrumNoteNumber(35 + i))
                        ->getIndivFxMixer()
                        ->setOutput(DrumMixerIndividualOutput(0));
                    drumBus->getIndivFxMixerChannels()[i]->setOutput(
                        DrumMixerIndividualOutput(0));
                }
            }
            for (int b = 0; b < 16; ++b)
            {
                render();
                validateVoices();
                for (int c = 0; c < buffer.getNumChannels(); ++c)
                {
                    for (int f = 0; f < frames; ++f)
                    {
                        const float sample = buffer.getSample(c, f);
                        if (!std::isfinite(sample))
                        {
                            throw std::runtime_error("Non-finite tail output");
                        }
                        if (c >= 2)
                        {
                            tailEnergy += std::abs(sample);
                        }
                        tailChannelEnergy[c] += double(sample) * sample;
                        uint32_t bits;
                        std::memcpy(&bits, &sample, sizeof(bits));
                        tailHashes[c] ^= bits;
                        tailHashes[c] *= 1099511628211ULL;
                    }
                }
            }
        }
        const auto warmStart = Clock::now();
        do
        {
            render();
            validateVoices();
        } while (
            std::chrono::duration<double>(Clock::now() - warmStart).count() <
            warmup);
        // Fixed storage avoids sample-vector growth contaminating
        // RAM/allocation results. Quantiles use a deterministic reservoir if a
        // run exceeds this capacity.
        std::vector<double> timings(65536, 0.0);
        json memoryBefore = memory();
        uint64_t callbacks = 0, overruns = 0, random = 0x12345678;
        double total = 0, maximum = 0;
        const double deadline = double(frames) / rate;
        const auto started = Clock::now();
        const auto cpuStart = cpuSeconds();
        benchmark::allocationCount = benchmark::allocationBytes = 0;
        do
        {
            buffer.clear();
            midi.clear();
            benchmark::countAllocations = allocations;
            const auto a = Clock::now();
            processor.processBlock(buffer, midi);
            const auto b = Clock::now();
            benchmark::countAllocations = false;
            const double elapsed = std::chrono::duration<double>(b - a).count();
            total += elapsed;
            maximum = std::max(maximum, elapsed);
            overruns += elapsed >= deadline;
            ++callbacks;
            if (callbacks <= timings.size())
            {
                timings[callbacks - 1] = elapsed;
            }
            else
            {
                random ^= random << 13;
                random ^= random >> 7;
                random ^= random << 17;
                const auto index = random % callbacks;
                if (index < timings.size())
                {
                    timings[index] = elapsed;
                }
            }
            if (callbacks % 128 == 0)
            {
                validateVoices();
            }
        } while (callbacks < request.value("minimum_callbacks", 2000) ||
                 std::chrono::duration<double>(Clock::now() - started).count() <
                     seconds);
        const auto cpu = cpuSeconds() - cpuStart;
        const auto memoryAfter = memory();
        validateVoices();
        double lastEnergy = 0;
        for (int c = 0; c < buffer.getNumChannels(); ++c)
        {
            for (int f = 0; f < frames; ++f)
            {
                const float value = buffer.getSample(c, f);
                if (!std::isfinite(value))
                {
                    throw std::runtime_error("Non-finite final output");
                }
                lastEnergy += double(value) * value;
            }
        }
        if (!idle && lastEnergy <= 1e-12)
        {
            throw std::runtime_error("Workload became silent");
        }
        timings.resize(std::min<uint64_t>(callbacks, timings.size()));
        std::sort(timings.begin(), timings.end());
        auto percentile = [&](double p)
        {
            return timings[std::min(
                       timings.size() - 1,
                       size_t(std::ceil(p * timings.size()) - 1))] *
                   1e6;
        };
        json channels = json::array(), tailChannels = json::array();
        for (int c = 0; c < buffer.getNumChannels(); ++c)
        {
            tailChannels.push_back({{"channel", c},
                                    {"hash", std::to_string(tailHashes[c])},
                                    {"energy", tailChannelEnergy[c]}});
            channels.push_back({{"channel", c},
                                {"hash", std::to_string(channelHashes[c])},
                                {"energy", channelEnergy[c]}});
        }
        json result = {
            {"revision", VMPC_BENCHMARK_REVISION},
            {"pointer_bytes", sizeof(void *)},
            {"request", request},
            {"callbacks", callbacks},
            {"quantile_samples", timings.size()},
            {"voices", activeVoices(processor)},
            {"audio_hash", std::to_string(hash)},
            {"audio_energy", energy},
            {"audio_channels", channels},
            {"tail_energy", tailEnergy},
            {"tail_channels", tailChannels},
            {"mean_us", total / callbacks * 1e6},
            {"p95_us", percentile(.95)},
            {"p99_us", percentile(.99)},
            {"maximum_us", maximum * 1e6},
            {"deadline_us", deadline * 1e6},
            {"overruns", overruns},
            {"thread_cpu_percent", cpu / (callbacks * deadline) * 100},
            {"memory_before", memoryBefore},
            {"memory_after", memoryAfter},
            {"allocations", benchmark::allocationCount},
            {"allocated_bytes", benchmark::allocationBytes}};
        processor.releaseResources();
        std::ofstream out(argv[2]);
        out << result.dump(2) << '\n';
        if (!out)
        {
            throw std::runtime_error("Cannot write worker result");
        }
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Benchmark worker failed: " << e.what() << '\n';
        return 2;
    }
}
