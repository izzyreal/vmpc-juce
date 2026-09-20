#include "Allocations.hpp"
#include <cstdlib>
#include <new>
#if defined(_WIN32)
#include <malloc.h>
#endif
namespace benchmark
{
    thread_local bool countAllocations = false;
    thread_local uint64_t allocationCount = 0;
    thread_local uint64_t allocationBytes = 0;
} // namespace benchmark
namespace
{
    void record(std::size_t n)
    {
        if (benchmark::countAllocations)
        {
            ++benchmark::allocationCount;
            benchmark::allocationBytes += n;
        }
    }
    void *allocate(std::size_t n)
    {
        if (auto *p = std::malloc(n ? n : 1))
        {
            record(n);
            return p;
        }
        throw std::bad_alloc();
    }
    void *alignedAllocate(std::size_t n, std::size_t alignment)
    {
        void *p = nullptr;
#if defined(_WIN32)
        p = _aligned_malloc(n ? n : 1, alignment);
#else
        if (posix_memalign(&p, alignment, n ? n : 1) != 0)
        {
            p = nullptr;
        }
#endif
        if (!p)
        {
            throw std::bad_alloc();
        }
        record(n);
        return p;
    }
    void alignedFree(void *p) noexcept
    {
#if defined(_WIN32)
        _aligned_free(p);
#else
        std::free(p);
#endif
    }
} // namespace
void *operator new(std::size_t n)
{
    return allocate(n);
}
void *operator new[](std::size_t n)
{
    return allocate(n);
}
void operator delete(void *p) noexcept
{
    std::free(p);
}
void operator delete[](void *p) noexcept
{
    std::free(p);
}
void operator delete(void *p, std::size_t) noexcept
{
    std::free(p);
}
void operator delete[](void *p, std::size_t) noexcept
{
    std::free(p);
}
void *operator new(std::size_t n, const std::nothrow_t &) noexcept
{
    try
    {
        return allocate(n);
    }
    catch (...)
    {
        return nullptr;
    }
}
void *operator new[](std::size_t n, const std::nothrow_t &) noexcept
{
    try
    {
        return allocate(n);
    }
    catch (...)
    {
        return nullptr;
    }
}
void operator delete(void *p, const std::nothrow_t &) noexcept
{
    std::free(p);
}
void operator delete[](void *p, const std::nothrow_t &) noexcept
{
    std::free(p);
}
void *operator new(std::size_t n, std::align_val_t a)
{
    return alignedAllocate(n, static_cast<std::size_t>(a));
}
void *operator new[](std::size_t n, std::align_val_t a)
{
    return alignedAllocate(n, static_cast<std::size_t>(a));
}
void operator delete(void *p, std::align_val_t) noexcept
{
    alignedFree(p);
}
void operator delete[](void *p, std::align_val_t) noexcept
{
    alignedFree(p);
}
void operator delete(void *p, std::size_t, std::align_val_t) noexcept
{
    alignedFree(p);
}
void operator delete[](void *p, std::size_t, std::align_val_t) noexcept
{
    alignedFree(p);
}
