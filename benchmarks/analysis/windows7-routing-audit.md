# Windows 7 routing discrepancy: source and binary audit

21 September 2026. The discrepancy is explained by a defective fallback in the
math routine linked into both Windows x86 workers. It is not explained by a
32-bit versus 64-bit layout difference. The same executable bytes pass on the
newer machine because the runtime selects a different instruction path.

## Evidence checked

The Windows 7 full report `20260921-054603-47cda844` and Windows 11 CI smoke
report `20260921-174202-8a6dab00` identify identical worker SHA-256 hashes:

- Before: `1d3f94886e4fd888a67c3da9575467ba6956bd5aa74a7e3203a06b392a25f78f`
- After: `d7e955038b47687d0f820fd26c37ff764573a7141a45bd93800427205d210d29`

Both are MSVC 19.42.34435.0, x86 Release, static CRT, `/arch:SSE2`, and have
the same dependency lock. The MPC source checkout is exactly the pinned
`99bfb9b65c2399edfdce5731036c6bf57f3ed176` revision.

In the Windows 7 report all main L/R hashes match across paired trials.
All twelve loaded `all` cases and all twelve loaded `spread` cases mismatch.
In `all`, the after worker's additional channels duplicate main L/R. In `spread`,
even the before worker duplicates main L/R across individual outputs; after
also copies that signal into the physical-sound bus. This pattern is important:
the old worker's output was already wrong where the filter allowed copying.

## Benchmark audit

Reviewed `Worker.cpp`, `Runner.cpp`, `Allocations.cpp`, the build overlay and
manifest, plus JUCE's owning `AudioBuffer` allocation and channel-offset code.

- The worker allocates an owning multichannel buffer. JUCE gives each channel
  distinct sample storage; the harness does not supply aliased external pointers.
- Bus layouts are configured before `prepareToPlay`, which computes the channel
  mappings. The fixture enables 6 stereo and 8 mono output buses, or main stereo
  alone. It uses a VST3 wrapper identity consistently on both platforms.
- Hashing reads each output channel directly. The energy factor follows the
  sample duplication; it is not a JSON/hash arithmetic interpretation error.
- Both versions get identical deterministic fixture requests in fresh processes.
  Worker identities, executable hashes and build settings are checked.
- Input/output clearing, separate sample storage, note triggering and allocation
  instrumentation do not explain a CPU-feature-dependent duplication of main
  L/R into every selected output.
- Environment isolation uses the same CRT environment API that MPC's `Paths.cpp`
  reads. No external audio device or host is involved.

This is a focused audit, not a proof that the entire harness is defect-free.
Two validation limitations matter: matching A/B hashes alone cannot detect a
bug shared by both versions, and the tail check's nonzero-energy condition can
pass for incorrect duplicated audio. The Windows 7 tail result therefore does
not independently prove correct tail routing. Future validation should include
expected silence on unused/physical outputs and a reference for intended routes.

## Production source and compiled failure

`editables/mpc/src/main/engine/audio/server/RealTimeAudioServer.cpp`, lines 89
and 107, computes stereo-buffer indices using:

```cpp
static_cast<int>(std::floor(mpcMonoOutputChannelIndices[i] / 2.f))
```

The input loop uses the same expression. The indices are small nonnegative
integers. There is no numerical ambiguity: 0..11 must map to
`0,0,1,1,2,2,3,3,4,4,5,5`.

Disassembly of the actual after worker shows the output calculation at
`0x006d5450..0x006d547e`. It computes the half-index as a float in XMM0,
converts it through x87 into a double stack argument, then calls the linked
`floor` routine at `0x008c17d0`. This is a valid stack argument; XMM0 still
contains the float encoding with zero upper bits.

The runtime dispatcher compares its ISA level against 2. Its CPU initializer
sets this level when the SSE4.2 CPUID bit is present. The higher-level path
loads the double argument from the stack and executes SSE4.1 `roundsd`:

```asm
008c17d0  cmp     dword ptr [01c2262c], 2
008c17d7  jb      008c1790
008c17d9  movq    xmm0, qword ptr [esp+4]
008c17df  roundsd xmm0, xmm0, 9
```

The lower-level path at `0x008c1790` checks SSE2 math availability and FP control
state, then jumps to `0x008c17f0` **without loading that stack argument**.
It uses XMM0's bits as a double. For example, float 1.0 is
`0x000000003f800000` in the low 64 bits: interpreted as a double it is a tiny
positive value, not 1.0. The fallback returns zero. Every positive half-index
in this loop suffers the same problem.

Thus every selected output reads `activeOutputs[0]`. The independently computed
`index % 2` still chooses L or R correctly. This explains exact alternating L/R
copies, not noise, corruption of the main signal, or a volume multiplier.

The before worker contains the same code defect at its corresponding addresses:
output calculation `0x006d8330..0x006d835e`, floor entry `0x008c1a30`.
The optimization's channel filter hides some wrong copies, particularly all
unused channels in `all` and the physical-sound channel in `spread`.

## Local reproduction without rebuilding or using Windows

[reproduce_windows_floor.py](reproduce_windows_floor.py) loads the original PE
sections into Unicorn 2.1.4 on this Mac and executes the original production
caller plus its linked math routine. It checks binary hashes before using
build-specific addresses. It does not launch the Windows application or alter
the executable. It supplies the channel byte, stack/register scaffolding and
runtime dispatch flags; it does not synthesize the floating-point argument.

Both executable files produced:

| Dispatch path | Stereo-buffer indices for mono indices 0..11 |
| --- | --- |
| Older CPU / ISA level 1 | `0,0,0,0,0,0,0,0,0,0,0,0` |
| Newer CPU / ISA level 2 | `0,0,1,1,2,2,3,3,4,4,5,5` |

All 48 index checks passed their expected reproduction assertions. This is
isolated instruction emulation with selected runtime flags, not a complete
Windows 7 emulation. Together with the disassembly and the observed per-channel
fingerprint, it establishes a concrete mechanism for the reported discrepancy.

For another analysis machine, install `unicorn==2.1.4` in a temporary Python
virtual environment and run:

```
python reproduce_windows_floor.py /path/to/before.exe /path/to/after.exe
```

On this Apple Silicon Mac the ARM-native Unicorn library initially worked but
subsequently trapped in its host `init_cache_info` routine, before executing
guest instructions. The final reproduction also passed using the x86_64 wheel
under Rosetta, avoiding that unrelated emulator initialization issue:

```
arch -x86_64 /usr/bin/python3 -m venv /tmp/vmpc-x86-audit-rosetta
arch -x86_64 /tmp/vmpc-x86-audit-rosetta/bin/python -m pip install unicorn==2.1.4
arch -x86_64 /tmp/vmpc-x86-audit-rosetta/bin/python reproduce_windows_floor.py /path/to/before.exe /path/to/after.exe
```

A separate [upstream QEMU issue](https://gitlab.com/qemu-project/qemu/-/issues/2817)
reports a similar Windows floor-to-zero symptom on older CPU configurations.
That report is corroborating context, not the basis for this diagnosis or a
vendor confirmation of which MSVC releases are affected.

## Targeted remedy and scope

Replace both stereo-index calculations with integer division:

```cpp
const auto mpcStereoInputIndex = mpcMonoInputChannelIndices[i] / 2;
const auto mpcStereoOutputIndex = mpcMonoOutputChannelIndices[i] / 2;
```

For the valid nonnegative channel-index domain this is exactly equivalent,
avoids the affected math routine, and removes unnecessary work from the callback.
The code change belongs in the shared MPC engine. A renewed A/B comparison
should apply the same engine fix to both revisions and record that dependency
change, so it still isolates removal of the output filter.

Other production uses of the linked floor routine may also be affected; this
workaround repairs these channel calculations, not the entire math library.
Choosing/verifying a corrected toolchain/runtime is a separate broader remedy.
The benchmark should additionally exercise the older runtime dispatch path in
local/CI validation rather than relying solely on a modern CPU smoke run.

No production or benchmark implementation changes were made during this audit.
The preserved artifacts are this report and the instruction-level reproducer.
A final old-machine sanity check remains useful after the fix, but the diagnosis
no longer depends on another trial-and-error transfer cycle.
