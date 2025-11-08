#pragma once

#include <type_traits>
#include "types.h"

namespace gstd { // yep also to follow rairai's impl
class GenericVector {
    public:
    void* field1 = nullptr;
    void* field2 = nullptr;
    void* field3 = nullptr;
};

template <typename T>
class vector {
    public:
    GenericVector gvector;

    // This includes a destructor
    ~vector() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (u32* current = (u32*)gvector.field1; current != (u32*)gvector.field2; current = (u32*)((u32)current + sizeof(T))) {
                reinterpret_cast<T*>(current)->~T();
            }
        }
    }
};
}