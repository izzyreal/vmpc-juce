#!/usr/bin/env python3
"""Emulate the shipped x86 channel-index calculation, without Windows or a rebuild.

Requires Python and unicorn==2.1.4 on the analysis machine only.
Usage: python reproduce_windows_floor.py /path/to/before.exe /path/to/after.exe
Addresses are tied to the SHA-256 hashes below. Other builds are rejected.
This loads PE sections into an emulator; it does not launch either application.
For Apple Silicon, see the Rosetta instructions in windows7-routing-audit.md.
"""
import argparse
import hashlib
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (
    UC_X86_REG_EBP, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_ESI,
    UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR,
)

BUILDS = {
    '1d3f94886e4fd888a67c3da9575467ba6956bd5aa74a7e3203a06b392a25f78f': {
        'label': 'before', 'start': 0x6D8330, 'end': 0x6D835E,
        'isa': 0x1C2362C, 'sse2_math': 0x1C249B8,
    },
    'd7e955038b47687d0f820fd26c37ff764573a7141a45bd93800427205d210d29': {
        'label': 'after', 'start': 0x6D5450, 'end': 0x6D547E,
        'isa': 0x1C2262C, 'sse2_math': 0x1C239B8,
    },
}


def load_image(data):
    def u16(offset):
        return struct.unpack_from('<H', data, offset)[0]

    def u32(offset):
        return struct.unpack_from('<I', data, offset)[0]

    pe = u32(0x3c)
    assert data[pe:pe + 4] == b'PE\0\0' and u16(pe + 4) == 0x14c
    optional = pe + 24
    assert u16(optional) == 0x10b  # PE32
    base, size = u32(optional + 28), u32(optional + 56)
    emulator = Uc(UC_ARCH_X86, UC_MODE_32)
    emulator.mem_map(base, (size + 4095) & ~4095)
    emulator.mem_write(base, data[:u32(optional + 60)])
    for i in range(u16(pe + 6)):
        section = optional + u16(pe + 20) + i * 40
        raw_start, raw_size = u32(section + 20), u32(section + 16)
        if raw_size:
            emulator.mem_write(base + u32(section + 12), data[raw_start:raw_start + raw_size])
    return emulator


def reproduce(path):
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest not in BUILDS:
        raise ValueError('Unrecognized executable: ' + digest)
    build = BUILDS[digest]
    print(build['label'], digest)
    expected = [n // 2 for n in range(12)]
    for isa in [1, 2]:
        emulator = load_image(data)
        stack = 0x3000000
        emulator.mem_map(stack, 0x10000)
        emulator.mem_write(build['isa'], struct.pack('<I', isa))
        emulator.mem_write(build['sse2_math'], struct.pack('<I', 1))
        actual = []
        for channel in range(12):
            emulator.reg_write(UC_X86_REG_ESP, stack + 0x8000)
            emulator.reg_write(UC_X86_REG_EBP, stack + 0x9000)
            emulator.reg_write(UC_X86_REG_ECX, 0)
            emulator.reg_write(UC_X86_REG_ESI, stack + 0x100)
            emulator.mem_write(stack + 0x100, bytes([channel]))
            # Masked FP exceptions; JUCE's flush-to-zero/denormals-are-zero bits.
            emulator.reg_write(UC_X86_REG_FPCW, 0x37f)
            emulator.reg_write(UC_X86_REG_MXCSR, 0x9fc0)
            # Execute the ORIGINAL production caller and its linked floor routine.
            # No synthetic floating-point argument or register value is supplied.
            emulator.emu_start(build['start'], build['end'], count=500)
            actual.append(emulator.reg_read(UC_X86_REG_EDI))
        print('  ISA dispatch', isa, 'stereo buffer indices:', actual)
        assert actual == ([0] * 12 if isa == 1 else expected), actual
    print('  Confirmed: SSE2 fallback fails; newer rounding path succeeds.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executables', nargs='+', type=Path)
    for executable in parser.parse_args().executables:
        reproduce(executable)
