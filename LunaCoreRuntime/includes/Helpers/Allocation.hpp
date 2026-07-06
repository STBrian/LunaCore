#pragma once

#include <stdlib.h>
#include <source_location>
#include <new>
#include <utility>
#include <memory>
#include <type_traits>

#include "Helpers/Errors.hpp"
#include "Core/CrashHandler.hpp"

#include "game/gmalloc.h"

namespace Core {

#define HEAP_PLUGIN 0
#define HEAP_GAME_APP 1
#define HEAP_GAME_LINEAR 2

template <typename T, typename... Args>
inline T* alloc(Args&&... args) {
    void* ptr = std::malloc(sizeof(T) + 8);
    if (ptr == nullptr) {
        ptr = gstd_malloc(sizeof(T) + 8); // try to get help from game's heap
        if (ptr == nullptr) CrashHandler::Abort(ErrorCode::Allocation_Error);
        *(u32*)ptr = HEAP_GAME_APP;
    } else
        *(u32*)ptr = HEAP_PLUGIN;
    void* p = (u32*)ptr + 2;
    return new (p) T(std::forward<Args>(args)...);
}

template <typename T>
inline T* alloc_array(size_t count) {
    void* memory = std::malloc(sizeof(T) * count + 0x8);
    if (memory == nullptr) CrashHandler::Abort("Allocation error array");

    *reinterpret_cast<u32*>(memory) = count;
    T* ptr = reinterpret_cast<T*>(reinterpret_cast<u32>(memory) + 0x8);

    for (size_t i = 0; i < count; i++) {
        new (&ptr[i]) T();
    }
    return ptr;
}

template <typename T, typename... Args>
T* alloc_raw(Args&&... args) {
    void* ptr = std::malloc(sizeof(T) + 8);
    if (ptr == nullptr) return nullptr; // don't try with game's allocator as it causes an abort when fails to reserve
    else *(u32*)ptr = HEAP_PLUGIN;
    void* p = (u32*)ptr + 2;
    return new (p) T(std::forward<Args>(args)...);
}

template <typename T>
T* alloc_array_raw(size_t count) {
    void* memory = std::malloc(sizeof(T) * count + 0x8);
    if (memory == nullptr) return nullptr;

    *reinterpret_cast<u32*>(memory) = count;
    T* ptr = reinterpret_cast<T*>(reinterpret_cast<u32>(memory) + 0x8);

    for (size_t i = 0; i < count; i++) {
        new (&ptr[i]) T();
    }
    return ptr;
}


template <typename T>
void dealloc(T* ptr) {
    if (ptr == nullptr) return;
    if constexpr (!std::is_trivially_destructible_v<T>) {
        ptr->~T();
    }
    u32* heapType = (u32*)ptr - 2;
    if (*heapType == HEAP_PLUGIN)
        std::free(heapType);
    else
        gstd_free(heapType);
}

template <typename T>
void dealloc_array(T* ptr) {
    if (ptr == nullptr) return;
    if constexpr (!std::is_trivially_destructible_v<T>) {
        u32 count = *(reinterpret_cast<u32*>(ptr) - 0x2);
        for (size_t i = 0; i < count; i++) {
            ptr[i].~T();
        }
    }
    std::free(reinterpret_cast<u32*>(ptr) - 0x2);
}

namespace UniqueAlloc {
    struct CustomDestructor {
        template <typename T>
        void operator()(T* ptr) const {
            dealloc<T>(ptr);
        }
    };

    struct CustomDestructorArray {
        template <typename T>
        void operator()(T* ptr) const {
            dealloc_array<T>(ptr);
        }
    };

    template <typename T, typename... Args>
    std::unique_ptr<T, CustomDestructor> alloc(Args&&... args) {
        return std::unique_ptr<T, CustomDestructor>(Core::alloc<T>(std::forward<Args>(args)...));
    }

    template <typename T, typename... Args>
    std::unique_ptr<T, CustomDestructorArray> alloc_array(Args&&... args) {
        return std::unique_ptr<T, CustomDestructorArray>(Core::alloc_array<T>(std::forward<Args>(args)...));
    }

    template <typename T, typename... Args>
    std::unique_ptr<T, CustomDestructor> alloc_raw(Args&&... args) {
        return std::unique_ptr<T, CustomDestructor>(Core::alloc_raw<T>(std::forward<Args>(args)...));
    }

    template <typename T, typename... Args>
    std::unique_ptr<T, CustomDestructorArray> alloc_array_raw(Args&&... args) {
        return std::unique_ptr<T, CustomDestructorArray>(Core::alloc_array_raw<T>(std::forward<Args>(args)...));
    }
}
}