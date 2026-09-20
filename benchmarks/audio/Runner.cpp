#include "Statistics.hpp"
#include "AudioValidation.hpp"
#include "Allocations.hpp"
#include <new>
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
using json = nlohmann::json;
static const char *beforeRevision = "eeb8d07c7d5e2166fc2d2a1dd2d3febf491dd010";
static const char *afterRevision = "a45d440806f43b9702947ca22cc648aa23cfa5eb";
static json read(const juce::File &file)
{
    std::ifstream input(file.getFullPathName().toStdString());
    json value;
    input >> value;
    return value;
}
static void write(const juce::File &file, const json &value)
{
    if (!file.replaceWithText(value.dump(2) + "\n"))
    {
        throw std::runtime_error("Cannot write " +
                                 file.getFullPathName().toStdString());
    }
}
static std::string caseName(const json &request)
{
    return std::to_string(request.at("rate").get<int>()) + "/" +
           std::to_string(request.at("frames").get<int>()) + " " +
           request.at("routing").get<std::string>() +
           (request.at("idle").get<bool>()
                ? " idle"
                : (request.at("mono").get<bool>() ? " mono32" : " stereo32")) +
           (request.at("drum").get<bool>() ? " drum" : " program");
}
static void selfTest()
{
    auto require = [](bool b)
    {
        if (!b)
        {
            throw std::runtime_error("Benchmark self-test failed");
        }
    };
    benchmark::allocationCount = benchmark::allocationBytes = 0;
    benchmark::countAllocations = true;
    void *ordinary = ::operator new(13);
    void *aligned = ::operator new(64, std::align_val_t(32));
    benchmark::countAllocations = false;
    ::operator delete(ordinary);
    ::operator delete(aligned, std::align_val_t(32));
    require(benchmark::allocationCount == 2 &&
            benchmark::allocationBytes == 77);
    require(benchmark::compare({100, 100, 100, 100, 100, 100},
                               {90, 90, 90, 90, 90, 90}, .05, 1)
                .verdict == "ACCEPTABLE");
    require(benchmark::compare({100, 100, 100, 100, 100, 100},
                               {120, 120, 120, 120, 120, 120}, .05, 1)
                .verdict == "REGRESSION");
    require(benchmark::compare({100, 100, 100, 100, 100, 100},
                               {80, 130, 90, 120, 100, 115}, .05, 1)
                .verdict == "INCONCLUSIVE");
    require(benchmark::compare({1, 1}, {1.1, 1.1}, .05, 1).verdict ==
            "ACCEPTABLE");
    require(benchmark::compare({100e6, 100e6}, {110e6, 110e6}, .05, 1048576)
                .verdict == "REGRESSION");
    require(benchmark::compare({100e6, 100e6}, {100.1e6, 100.1e6}, .05, 1048576)
                .verdict == "ACCEPTABLE");
    require(benchmark::headroom(10, 100, 0, .5) == "COMFORTABLE");
    require(benchmark::headroom(60, 100, 0, .5) == "LIMITED");
    require(benchmark::headroom(120, 100, 1, .5) == "INSUFFICIENT");
    require(benchmark::headroom(10, 100, 1, .5) == "RECHECK OVERRUNS");
    bool rejected = false;
    try
    {
        benchmark::compare({1}, {1, 2}, .05, 1);
    }
    catch (...)
    {
        rejected = true;
    }
    require(rejected);
    json channels = json::array();
    for (int c = 0; c < 20; ++c)
    {
        channels.push_back(
            {{"energy", c < 2 ? 1.0 : 0.0}, {"hash", std::to_string(c)}});
    }
    const json request = {{"routing", "all"}, {"idle", false}};
    require(benchmark::routingValid(channels, request));
    // Identical A/B hashes must not excuse a duplicated main signal.
    channels[2] = channels[0];
    require(!benchmark::routingValid(channels, request));
    for (int c = 2; c < 20; ++c)
    {
        channels[c]["energy"] = c == 10 || c == 11 ? 0.0 : 1.0;
        channels[c]["hash"] = std::to_string(c);
    }
    for (int c = 0; c < 8; ++c)
    {
        channels[12 + c] = channels[2 + c];
    }
    require(benchmark::routingValid(channels, {{"routing", "spread"}}));
    require(benchmark::tailMatches(channels, channels));
    auto wrongTail = channels;
    wrongTail[2] = channels[0];
    require(!benchmark::tailMatches(wrongTail, channels));
    channels[10]["energy"] = 1.0;
    require(!benchmark::tailMatches(channels, channels));
    require(!benchmark::routingValid(channels, {{"routing", "spread"}}));
    std::cout << "Statistics and audio validation self-tests passed.\n";
}
int main(int argc, char **argv)
{
    try
    {
        bool smoke = false, extended = false;
        std::vector<std::string> caseFilters;
        double relativeLimit = .05, deadlineLimit = .01, memoryLimit = 1048576,
               headroomLimit = .5;
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            if (arg == "--self-test")
            {
                selfTest();
                return 0;
            }
            if (arg == "--smoke")
            {
                smoke = true;
            }
            else if (arg == "--extended")
            {
                extended = true;
            }
            else if (arg == "--case" && i + 1 < argc)
            {
                caseFilters.emplace_back(argv[++i]);
            }
            else if ((arg == "--relative-limit" || arg == "--deadline-limit" ||
                      arg == "--memory-limit-mib" ||
                      arg == "--headroom-limit") &&
                     i + 1 < argc)
            {
                const double value = std::stod(argv[++i]);
                if (!std::isfinite(value) || value <= 0)
                {
                    throw std::runtime_error(
                        "Limits must be positive and finite");
                }
                if (arg == "--relative-limit")
                {
                    relativeLimit = value / 100;
                }
                if (arg == "--deadline-limit")
                {
                    deadlineLimit = value / 100;
                }
                if (arg == "--memory-limit-mib")
                {
                    memoryLimit = value * 1048576;
                }
                if (arg == "--headroom-limit")
                {
                    headroomLimit = value / 100;
                }
            }
            else if (arg == "--help")
            {
                std::cout
                    << "Usage: vmpc-audio-benchmark [--smoke | --extended] "
                       "[--case \"CASE NAME\"]\n"
                       "  --relative-limit PERCENT (5)  --deadline-limit "
                       "PERCENT (1)\n"
                       "  --memory-limit-mib MiB (1)    --headroom-limit "
                       "PERCENT (50)\n"
                       "Run on an otherwise idle system connected to AC power. "
                       "Results are saved beside this executable.\n";
                return 0;
            }
            else
            {
                throw std::runtime_error("Unknown option: " + arg);
            }
        }
        const auto home =
            juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                .getParentDirectory();
        const auto manifest = read(home.getChildFile("manifest.json"));
        if (manifest.at("revisions").at("before") != beforeRevision ||
            manifest.at("revisions").at("after") != afterRevision)
        {
            throw std::runtime_error("Unexpected source revisions in manifest");
        }
        const auto &builds = manifest.at("builds");
        for (const auto *key : {"compiler", "config", "pointer_bytes",
                                "cmake_options", "effective_flags"})
        {
            if (builds.at("before").at(key) != builds.at("after").at(key))
            {
                throw std::runtime_error("Build setting mismatch");
            }
        }
        if (builds.at("before").at("config") != "Release")
        {
            throw std::runtime_error("Release builds required");
        }
        const auto directory = home.getChildFile("results").getChildFile(
            juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S") + "-" +
            juce::String::toHexString(
                juce::Random::getSystemRandom().nextInt()));
        if (!directory.createDirectory())
        {
            throw std::runtime_error("Cannot create results directory");
        }
        std::ostringstream report;
        auto line = [&](const std::string &value)
        {
            std::cout << value << std::endl;
            report << value << '\n';
        };
        line("VMPC audio comparison: eeb8d07 (before) -> a45d440 (after)");
        line("System: " +
             juce::SystemStats::getOperatingSystemName().toStdString() + "; " +
             juce::SystemStats::getCpuModel().toStdString() + "; " +
             std::to_string(juce::SystemStats::getNumCpus()) +
             " logical CPUs; " +
             std::to_string(juce::SystemStats::getMemorySizeInMegabytes()) +
             " MiB RAM");
        line(
            "CPU load is percent of ONE audio thread's time budget, not "
            "percent of all cores.");
        line("Limits: relative " + std::to_string(relativeLimit * 100) +
             "%; deadline " + std::to_string(deadlineLimit * 100) +
             " percentage points; RAM " +
             std::to_string(memoryLimit / 1048576) +
             " MiB AND relative limit; p99 headroom " +
             std::to_string(headroomLimit * 100) + "%.");
        line(smoke
                 ? "SMOKE TEST: functional checks only; no performance verdict."
                 : "Default suite takes roughly 20-30 minutes; extended runs "
                   "take longer. Keep other applications idle.");
        json output = {
            {"manifest", manifest},
            {"smoke", smoke},
            {"system",
             {{"os", juce::SystemStats::getOperatingSystemName().toStdString()},
              {"cpu", juce::SystemStats::getCpuModel().toStdString()},
              {"logical_cpus", juce::SystemStats::getNumCpus()},
              {"ram_mib", juce::SystemStats::getMemorySizeInMegabytes()}}},
            {"thresholds",
             {{"relative", relativeLimit},
              {"deadline_fraction", deadlineLimit},
              {"memory_bytes", memoryLimit},
              {"headroom_fraction", headroomLimit}}},
            {"cases", json::array()}};
        juce::File workers[2];
        for (int variant = 0; variant < 2; ++variant)
        {
            const std::string label = variant ? "after" : "before";
#if JUCE_MAC
            workers[variant] = home.getChildFile(
                label + ".app/Contents/MacOS/vmpc-audio-worker");
#elif JUCE_WINDOWS
            workers[variant] = home.getChildFile(label + ".exe");
#else
            workers[variant] = home.getChildFile(label);
#endif
            if (!workers[variant].existsAsFile() ||
                juce::SHA256(workers[variant]).toHexString().toStdString() !=
                    builds.at(label).at("binary_sha256"))
            {
                throw std::runtime_error("Missing or modified " + label +
                                         " executable");
            }
        }
        int invocation = 0;
        auto worker = [&](int variant, json request)
        {
            const auto stem =
                juce::String(++invocation) + (variant ? "-after" : "-before");
            const auto input = directory.getChildFile(stem + "-request.json");
            const auto resultFile =
                directory.getChildFile(stem + "-result.json");
            write(input, request);
            juce::ChildProcess process;
            if (!process.start(juce::StringArray{
                    workers[variant].getFullPathName(), input.getFullPathName(),
                    resultFile.getFullPathName()}))
            {
                throw std::runtime_error("Could not start worker");
            }
            // JUCE pipe reads block. A reader thread drains output while the
            // main thread enforces the timeout, including for a completely
            // silent worker.
            std::string log;
            std::thread reader(
                [&]
                {
                    log = process.readAllProcessOutput().toStdString();
                });
            const bool finished = process.waitForProcessToFinish(180000);
            if (!finished)
            {
                process.kill();
            }
            reader.join();
            if (!finished)
            {
                throw std::runtime_error("Worker timed out");
            }
            directory.getChildFile(stem + ".log").replaceWithText(log);
            if (process.getExitCode() != 0 || !resultFile.existsAsFile())
            {
                throw std::runtime_error("Worker failed: " + log);
            }
            auto result = read(resultFile);
            if (result.at("revision") !=
                    (variant ? afterRevision : beforeRevision) ||
                result.at("pointer_bytes") !=
                    builds.at(variant ? "after" : "before")
                        .at("pointer_bytes") ||
                result.at("request") != request)
            {
                throw std::runtime_error("Worker identity/request mismatch");
            }
            if (result.at("voices") != (request.value("idle", false) ? 0 : 32))
            {
                throw std::runtime_error("Invalid voice count");
            }
            return result;
        };
        std::vector<json> cases;
        for (int rate : extended ? std::vector<int>{44100, 48000, 96000}
                                 : std::vector<int>{48000})
        {
            for (int frames : smoke ? std::vector<int>{64}
                              : extended
                                  ? std::vector<int>{64, 128, 256, 512, 1024}
                                  : std::vector<int>{64, 256, 1024})
            {
                for (const std::string routing : {"stereo", "all", "spread"})
                {
                    for (bool mono : {true, false})
                    {
                        for (bool drum : {false, true})
                        {
                            cases.push_back({{"rate", rate},
                                             {"frames", frames},
                                             {"routing", routing},
                                             {"mono", mono},
                                             {"drum", drum},
                                             {"idle", false}});
                        }
                    }
                }
                cases.push_back({{"rate", rate},
                                 {"frames", frames},
                                 {"routing", "all"},
                                 {"mono", true},
                                 {"drum", false},
                                 {"idle", true}});
            }
        }
        if (!caseFilters.empty())
        {
            for (const auto &filter : caseFilters)
            {
                if (std::none_of(cases.begin(), cases.end(),
                                 [&](const auto &request)
                                 {
                                     return caseName(request) == filter;
                                 }))
                {
                    throw std::runtime_error("Unknown case in this suite: " +
                                             filter);
                }
            }
            cases.erase(std::remove_if(cases.begin(), cases.end(),
                                       [&](const auto &request)
                                       {
                                           return std::find(
                                                      caseFilters.begin(),
                                                      caseFilters.end(),
                                                      caseName(request)) ==
                                                  caseFilters.end();
                                       }),
                        cases.end());
            line("Selected " + std::to_string(cases.size()) +
                 " cases only; verdict applies to this subset.");
        }
        output["case_filters"] = caseFilters;
        output["extended"] = extended;
        bool regression = false, inconclusive = false, comfortable = true;
        bool audioMismatch = false;
        int index = 0;
        for (auto request : cases)
        {
            const std::string name = caseName(request);
            line("[" + std::to_string(++index) + "/" +
                 std::to_string(cases.size()) + "] " + name);
            request["seconds"] = smoke ? .03 : 2.;
            request["warmup"] = smoke ? .01 : .5;
            request["minimum_callbacks"] = smoke ? 32 : 2000;
            json trials[2] = {json::array(), json::array()};
            json mismatchedPairs = json::array();
            json invalidRoutingPairs = json::array();
            const int repetitions = smoke ? 2 : 6;
            for (int repeat = 0; repeat < repetitions; ++repeat)
            {
                for (int j = 0; j < 2; ++j)
                {
                    const int variant = (repeat + j) % 2;
                    trials[variant].push_back(worker(variant, request));
                }
                const bool matches = trials[0].back().at("audio_hash") ==
                                     trials[1].back().at("audio_hash");
                const bool routingValid =
                    benchmark::routingValid(
                        trials[0].back().at("audio_channels"), request) &&
                    benchmark::routingValid(
                        trials[1].back().at("audio_channels"), request);
                if (!matches || !routingValid)
                {
                    audioMismatch = true;
                    if (!matches)
                    {
                        mismatchedPairs.push_back(repeat + 1);
                    }
                    if (!routingValid)
                    {
                        invalidRoutingPairs.push_back(repeat + 1);
                    }
                    line("AUDIO VALIDATION FAILED: " + name + ", pair " +
                         std::to_string(repeat + 1) +
                         ". Continuing diagnostics; no valid overall verdict.");
                    for (int v = 0; v < 2; ++v)
                    {
                        const auto &trial = trials[v].back();
                        line(std::string(v == 0 ? "  Before" : "  After") +
                             " channel diagnostics: " +
                             trial.value("audio_channels", json::array())
                                 .dump());
                    }
                }
                std::cout << "  pair " << repeat + 1 << '/' << repetitions
                          << " complete\n"
                          << std::flush;
            }
            const double deadline = trials[0][0].at("deadline_us");
            json comparisons;
            auto compareMetric =
                [&](const std::string &key, double absolute, bool memoryMetric)
            {
                std::vector<double> values[2];
                for (int v = 0; v < 2; ++v)
                {
                    for (const auto &trial : trials[v])
                    {
                        values[v].push_back(
                            memoryMetric
                                ? trial.at("memory_after").at(key).get<double>()
                                : trial.at(key).get<double>());
                    }
                }
                const auto c = benchmark::compare(values[0], values[1],
                                                  relativeLimit, absolute);
                comparisons[key] = {
                    {"before", c.before},
                    {"after", c.after},
                    {"delta", c.after - c.before},
                    {"delta_percent", c.deltaPercent},
                    {"threshold_margin_ci95", {c.low, c.high}},
                    {"verdict", smoke ? "SMOKE ONLY" : c.verdict}};
                if (!smoke)
                {
                    regression |= c.verdict == "REGRESSION";
                    inconclusive |= c.verdict == "INCONCLUSIVE";
                }
                return c;
            };
            const auto mean =
                compareMetric("mean_us", deadline * deadlineLimit, false);
            const auto cpu =
                compareMetric("thread_cpu_percent", deadlineLimit * 100, false);
            const auto ram = compareMetric("resident_bytes", memoryLimit, true);
            if (!trials[0][0].at("memory_after").at("private_bytes").is_null())
            {
                compareMetric("private_bytes", memoryLimit, true);
            }
            double p99 = 0, maxTime = 0, p95 = 0, beforeP99 = 0, beforeP95 = 0,
                   beforeMax = 0;
            uint64_t beforeOverruns = 0;
            for (const auto &trial : trials[0])
            {
                beforeOverruns += trial.at("overruns").get<uint64_t>();
                beforeP99 =
                    std::max(beforeP99, trial.at("p99_us").get<double>());
                beforeP95 =
                    std::max(beforeP95, trial.at("p95_us").get<double>());
                beforeMax =
                    std::max(beforeMax, trial.at("maximum_us").get<double>());
            }
            uint64_t overruns = 0;
            for (const auto &trial : trials[1])
            {
                p99 = std::max(p99, trial.at("p99_us").get<double>());
                p95 = std::max(p95, trial.at("p95_us").get<double>());
                maxTime =
                    std::max(maxTime, trial.at("maximum_us").get<double>());
                overruns += trial.at("overruns").get<uint64_t>();
            }
            const auto headroom =
                smoke ? "SMOKE ONLY"
                      : benchmark::headroom(p99, deadline, overruns,
                                            headroomLimit);
            comfortable &= headroom == "COMFORTABLE";
            auto allocationRequest = request;
            allocationRequest["allocations"] = true;
            allocationRequest["seconds"] = .02;
            allocationRequest["minimum_callbacks"] = 256;
            json allocationResults = {worker(0, allocationRequest),
                                      worker(1, allocationRequest)};
            const bool newAllocations =
                allocationResults[0].at("allocations") == 0 &&
                allocationResults[1].at("allocations").get<uint64_t>() > 0;
            if (!smoke)
            {
                regression |= newAllocations;
            }
            std::ostringstream row;
            row << std::fixed << std::setprecision(2)
                << "  Callback: " << mean.before << " -> " << mean.after
                << " us (" << std::showpos << mean.deltaPercent << "%"
                << std::noshowpos << "); deadline " << deadline << " us; "
                << (smoke ? "SMOKE ONLY" : mean.verdict) << '\n'
                << "  Thread CPU: " << cpu.before << " -> " << cpu.after
                << "% of one core; " << (smoke ? "SMOKE ONLY" : cpu.verdict)
                << '\n'
                << "  RAM: " << ram.before / 1048576 << " -> "
                << ram.after / 1048576 << " MiB (" << std::showpos
                << (ram.after - ram.before) << " bytes, " << ram.deltaPercent
                << "%" << std::noshowpos << "); "
                << (smoke ? "SMOKE ONLY" : ram.verdict) << '\n'
                << "  Before p95/p99/max: " << beforeP95 << '/' << beforeP99
                << '/' << beforeMax << " us" << '\n'
                << "  After p95/p99/max: " << p95 << '/' << p99 << '/'
                << maxTime << " us; p99 " << 100 * p99 / deadline
                << "% of deadline; overruns before/after " << beforeOverruns
                << "/" << overruns << "; " << headroom << '\n'
                << "  C++ allocations: "
                << allocationResults[0].at("allocations") << " / "
                << allocationResults[0].at("callbacks") << " callbacks before, "
                << allocationResults[1].at("allocations") << " / "
                << allocationResults[1].at("callbacks") << " after";
            row << "\n  Callback threshold-margin 95% CI: [" << mean.low << ", "
                << mean.high << "] us (positive = above regression threshold)";
            if (comparisons.contains("private_bytes"))
            {
                row << "\n  Private committed RAM: "
                    << comparisons["private_bytes"]["before"].get<double>() /
                           1048576
                    << " -> "
                    << comparisons["private_bytes"]["after"].get<double>() /
                           1048576
                    << " MiB; "
                    << comparisons["private_bytes"]["verdict"]
                           .get<std::string>();
            }
            line(row.str());
            output["cases"].push_back(
                {{"name", name},
                 {"audio_matches", mismatchedPairs.empty()},
                 {"audio_mismatched_pairs", mismatchedPairs},
                 {"routing_valid", invalidRoutingPairs.empty()},
                 {"invalid_routing_pairs", invalidRoutingPairs},
                 {"comparisons", comparisons},
                 {"headroom", headroom},
                 {"trials_before", trials[0]},
                 {"trials_after", trials[1]},
                 {"allocation_runs", allocationResults}});
            write(directory.getChildFile("report.json"), output);
            directory.getChildFile("report.txt").replaceWithText(report.str());
        }
        json tail = {{"rate", 48000},       {"frames", 64},
                     {"routing", "spread"}, {"mono", true},
                     {"drum", false},       {"tail", true},
                     {"idle", false},       {"seconds", .02},
                     {"warmup", .01},       {"minimum_callbacks", 32}};
        const auto oldTail = worker(0, tail), newTail = worker(1, tail);
        tail["tail"] = false;
        tail["tail_reference"] = true;
        const auto reference = worker(1, tail);
        const bool tailValid = benchmark::tailMatches(
            newTail.at("tail_channels"), reference.at("tail_channels"));
        output["tail_check"] = {{"before", oldTail},
                                {"after", newTail},
                                {"reference", reference},
                                {"valid", tailValid}};
        audioMismatch |= !tailValid;
        line(tailValid ? "Routing-tail check: individual channels match the "
                         "unchanged-routing reference."
                       : "AUDIO VALIDATION FAILED: routed tail differs from "
                         "its reference.");
        const std::string verdict =
            audioMismatch
                ? "INVALID: audio validation failed; performance results are "
                  "diagnostic only"
            : smoke        ? "SMOKE PASSED (no performance verdict)"
            : regression   ? "REGRESSION: inspect flagged cases"
            : inconclusive ? "INCONCLUSIVE: repeat the run"
                           : "ACCEPTABLE: no meaningful measured overhead";
        output["verdict"] = verdict;
        output["audio_matches"] = !audioMismatch;
        output["comfortable_headroom_all_cases"] =
            !audioMismatch && !smoke && comfortable;
        line("\nChange overhead: " + verdict);
        if (!smoke)
        {
            line(comfortable
                     ? "Machine headroom: COMFORTABLE in all tested cases."
                     : "Machine headroom: REVIEW cases marked LIMITED, "
                       "INSUFFICIENT, or RECHECK OVERRUNS.");
        }
        line(
            "CPU includes callback-loop bookkeeping; callback wall timings "
            "exclude setup/validation. Quantiles use up to 65,536 reservoir "
            "samples per trial.");
        line(
            "RAM is whole-worker process memory; C++ allocation counts exclude "
            "malloc/OS allocators. Overruns are offline observations, not "
            "measured audio dropouts.");
        line("Results: " + directory.getFullPathName().toStdString());
        write(directory.getChildFile("report.json"), output);
        directory.getChildFile("report.txt").replaceWithText(report.str());
        return audioMismatch ? 2 : 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: " << e.what() << "\nNo valid overall verdict.\n";
        return 2;
    }
}
