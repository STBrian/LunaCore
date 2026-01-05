/**
 * Code for allocator and vector was taken from
 * https://github.com/rairai6895/MC3DSPluginFramework/blob/main/Includes/gstd_alloc.hpp
 */

#pragma once

#if __STDC_HOSTED__
#include <vector>
#endif

#include <type_traits>
#include "game/types.h"
#include "game/gstdlib.h"

namespace gstd { // yep also to follow rairai's impl
class GenericVector {
    public:
    void* field1 = nullptr;
    void* field2 = nullptr;
    void* field3 = nullptr;
};

#if __STDC_HOSTED__

template <typename T>
class allocator {
  public:
    using value_type = T;

    T *allocate(std::size_t n) {
        return (T *)(gstd::malloc(sizeof(T) * n));
    }

    void deallocate(T *ptr, std::size_t n) {
        gstd::free((void *)ptr);
    }

    bool operator==(const allocator &) const { return true; }
    bool operator!=(const allocator &) const { return false; }
};

template <typename T>
using vector = std::vector<T, gstd::allocator<T>>;

#else

/* 
I'll leave this class for historical purposes but looks like std::vector works
fine with the game using its allocator.
Also fallback if using in freestanding
*/
template <typename T>
class vector {
    public:
    GenericVector gvector;

    // This includes a destructor
    ~gvector() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (u32* current = (u32*)gvector.field1; current != (u32*)gvector.field2; current = (u32*)((u32)current + sizeof(T))) {
                reinterpret_cast<T*>(current)->~T();
            }
        }
    }
};

#endif
}