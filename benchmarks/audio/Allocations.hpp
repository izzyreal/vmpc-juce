#pragma once
#include <cstdint>
namespace benchmark
{
    extern thread_local bool countAllocations;
    extern thread_local uint64_t allocationCount;
    extern thread_local uint64_t allocationBytes;
} // namespace benchmark
