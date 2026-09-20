# VMPC audio A/B benchmark

Extract the entire ZIP to a writable folder. On **Windows 7 SP1 or later,
32-bit x86 with SSE2**, double-click `run-benchmark.cmd`. No installation,
compiler, Python, Git, DAW, or audio device is needed. Use AC power and close
other busy applications. Leave the console open until the summary appears.
On macOS run `./vmpc-audio-benchmark` from Terminal.

The default suite takes roughly 20–30 minutes (longer on slow systems). It
compares six alternating pairs of separate worker processes for each case:
48 kHz, 64/256/1024 frames, mono/stereo samples, program/drum mixer settings,
stereo-only/all-enabled/spread individual routing, plus idle controls. All
non-idle cases maintain exactly 32 active voices playing separate one-second
samples with filtering and resampling (about 5.4 MiB mono / 10.8 MiB stereo
sample data).
The benchmark renders offline as fast as possible; it does not open an audio
device, simulate a DAW, or measure actual audio dropouts.

`--extended` adds 44.1/96 kHz and 128/512 frames (roughly five times as long).
`--smoke` runs short functional checks and NEVER produces a performance verdict.
`--self-test` tests reporting arithmetic and audio validation. `--help` lists threshold overrides.
To repeat a flagged case, copy its exact name from the report:

```
vmpc-audio-benchmark --case "48000/64 all mono32 drum"
```

Repeat `--case` to select multiple cases. Include `--extended` when selecting
an extended-only sample rate or buffer size. A filtered verdict applies only
to the selected cases; it does not replace the full-suite report.

## Reading the results

`results/<run>/report.txt` contains the readable report. `report.json` includes
all trial data, paired bootstrap intervals, thresholds, machine details and
build manifest. Per-worker requests, results and logs permit diagnosis.

**Change overhead** and **machine headroom** are separate conclusions. Default
CPU regression thresholds require BOTH more than 5% relative growth and more
than one percentage point of the audio callback deadline. RAM growth must
exceed BOTH 5% and 1 MiB. A 95% paired-bootstrap interval of the threshold
margin entirely above zero flags a regression; crossing zero is inconclusive.
Six pairs are a practical regression check, not a formal guarantee; repeat
inconclusive/noisy runs. Any newly introduced steady-state C++ allocation is
also flagged. The command-line threshold overrides are percentages except
`--memory-limit-mib`.

Headroom is comfortable when p99 is below 50% of the deadline and no callbacks
exceed the deadline. Limited/insufficient headroom and isolated overruns are
reported separately. A faster revision can still have insufficient headroom
on a particular machine. The maximum and overruns are exact; p95/p99 use a
deterministic reservoir of at most 65,536 callbacks per repetition.

CPU is audio-thread CPU seconds per rendered audio second, including minor
loop bookkeeping. It is NOT divided by the machine's core count. Wall times
measure `processBlock` only. Memory is whole-worker resident memory (plus
private committed bytes on Windows), measured after warmup and after timing;
peak resident memory is retained in JSON. It includes the identical harness,
samples, and preallocated measurement storage. Separate allocation passes
count callback-thread C++ new/new[] (including aligned allocation), not malloc,
OS allocators or other threads. The fixed timing storage is about 512 KiB.

Audio hashes must match across revisions for stable routing. Expected unused
channels must also be silent, and individual stereo/mono output aliases must
match, even when both revisions produce identical audio. A separate test
compares every individual channel's tail against an unchanged-routing reference
after assignments change; nonzero energy alone cannot pass this check.
The old revision's known tail bug is not a timing failure.
Stable-routing mismatches and invalid channel routing are recorded per pair, with per-channel hashes and
energy in the worker results. The runner continues through the remaining cases
to collect diagnostics, but ends with an INVALID verdict and exit code 2.
Performance comparisons from such a run cannot establish that the change is
acceptable. Worker crashes and malformed outputs still stop the run immediately.

## Building

Builder requirements: Python 3.9+, Git, CMake 3.24+, and a C++17 toolchain.
Windows uses Visual Studio 2022 with Win32 C++ support; the macOS build uses
Ninja and AppleClang. From the repository root:

```
python3 benchmarks/audio/build.py --smoke
```

On Windows use `python` instead. `--generator`, `--jobs` and `--output` can be
specified. Artifacts default to `build/audio-benchmark/`. The manual
`audio-benchmark` pipeline builds the Windows 7 Win32 ZIP in CI.
On Windows the builder temporarily maps its output directory to a free drive
letter using `subst`, so nested MSBuild paths stay short. It removes the mapping
when finished, including on build failure; artifacts remain in the requested
output directory. A CMake build cached under a different path is regenerated.

The script exports the two exact historical application revisions, overlays
the same harness/CMake integration, and uses `dependencies.json` to pin every
dependency. Existing local Git objects avoid downloads; otherwise they are
fetched by immutable hash. It never checks out a different branch in your
working tree. Both workers use identical Release flags and dependencies.
Both use the pinned MPC engine **with the same integer channel-index fix**:
the builder replaces the two floating-point `floor(index / 2.f)` expressions
with integer division. The dependency lock and packaged manifest record the
original revision and exact LF-normalized source hashes before/after this transformation;
Git CRLF line endings are normalized, and unexpected source changes fail the build. This avoids the faulty MSVC x86
`floor` runtime path observed on the older CPU, while keeping the optimization
removal as the only production difference between the workers. It does not
require publishing a separate MPC commit before building the benchmark.
A production audio-server regression executable checks all 12 input/output
channels, reversed host mappings, and four buffer sizes for each built revision.
Previous Windows runs with incorrect routing are not a valid performance baseline. The build overlay
disables ASIO in both versions because no audio device is opened; this also
avoids downloading an unnecessary SDK. Windows also disables optional external
Kaitai zlib/iconv libraries; the fixture generates samples directly and uses
Windows-native string conversion. The processor callback is unmodified.
Normal builds leave `VMPC_BUILD_AUDIO_BENCHMARK` off. Enabling it directly
builds a worker for the current checkout, useful for development; the packaged
comparison always uses the fixed historical revisions.

CI verifies 32-bit PE headers, static-runtime/system-DLL dependencies, runs
functional checks, and publishes import listings. Actual Windows 7 startup
must also be checked on the target machine; modern CI is not a Windows 7
compatibility emulator. Full-run timings are deliberately not a CI gate.
