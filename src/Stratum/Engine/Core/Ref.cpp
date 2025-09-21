#include "Ref.h"

#include <cstdlib>
#include <new>
#include <iostream>
#include <atomic>

// This is ugly!
// Just here so i can debug frame by frame allocation
// Will remove later

size_t totalAllocated = 0;
size_t totalAllocations = 0;
size_t freedAllocations = 0;
std::atomic_uint64_t gUsedMemory = 0;

void resetTotalAllocs()
{
    totalAllocated = 0;
    totalAllocations = 0;
    freedAllocations = 0;
}

// #define DEBUG

#ifdef DEBUG

void* operator new(std::size_t size) {
    totalAllocated += size;
    totalAllocations++;
    if (void* ptr = std::malloc(size + 8))
    {
        auto p = reinterpret_cast<size_t*>(ptr);
        p[0] = size;
        gUsedMemory += size;
        return p + 1;
    }
    throw std::bad_alloc();
}

void operator delete(void* ptr) noexcept {
    auto p = reinterpret_cast<size_t*>(ptr) - 1;
    freedAllocations++;
    gUsedMemory -= p[0];
    std::free(p);
}

#endif