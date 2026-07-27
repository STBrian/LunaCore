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

#define BLOCK_HEADER_SIZE 0x8 

template <typename T, typename... Args>
inline T* alloc(Args&&... args) {
    const u32 blockSize = sizeof(T) + BLOCK_HEADER_SIZE;
    u32 heapType = HEAP_PLUGIN;

    void* ptr = std::malloc(blockSize);
    if (ptr == nullptr) {
        ptr = gstd_malloc(blockSize); // try to get help from game's heap
        if (ptr == nullptr) CrashHandler::Abort(ErrorCode::Allocation_Error);
        heapType = HEAP_GAME_APP;
    }

    u32* blockHeader = reinterpret_cast<u32*>(ptr);
    blockHeader[0] = heapType;
    blockHeader[1] = 1; // count

    void* p = reinterpret_cast<void*>(reinterpret_cast<u32>(ptr) + BLOCK_HEADER_SIZE);
    return new (p) T(std::forward<Args>(args)...);
}

template <typename T>
inline T* alloc_array(size_t count) {
    const u32 blockSize = sizeof(T) * count + BLOCK_HEADER_SIZE;
    u32 heapType = HEAP_PLUGIN;

    void* memory = std::malloc(blockSize);
    if (memory == nullptr) CrashHandler::Abort("Allocation error array");

    u32* blockHeader = reinterpret_cast<u32*>(memory);
    blockHeader[0] = heapType;
    blockHeader[1] = count;

    T* ptr = reinterpret_cast<T*>(reinterpret_cast<u32>(memory) + BLOCK_HEADER_SIZE);
    for (size_t i = 0; i < count; i++) {
        new (&ptr[i]) T();
    }
    return ptr;
}

template <typename T, typename... Args>
T* alloc_raw(Args&&... args) {
    const u32 blockSize = sizeof(T) + BLOCK_HEADER_SIZE;
    u32 heapType = HEAP_PLUGIN;

    void* ptr = std::malloc(blockSize);
    if (ptr == nullptr) return nullptr;

    u32* blockHeader = reinterpret_cast<u32*>(ptr);
    blockHeader[0] = heapType;
    blockHeader[1] = 1;

    void* p = reinterpret_cast<void*>(reinterpret_cast<u32>(ptr) + BLOCK_HEADER_SIZE);
    return new (p) T(std::forward<Args>(args)...);
}

template <typename T>
T* alloc_array_raw(size_t count) {
    const u32 blockSize = sizeof(T) * count + BLOCK_HEADER_SIZE;
    u32 heapType = HEAP_PLUGIN;

    void* memory = std::malloc(blockSize);
    if (memory == nullptr) return nullptr;;

    u32* blockHeader = reinterpret_cast<u32*>(memory);
    blockHeader[0] = heapType;
    blockHeader[1] = count;

    T* ptr = reinterpret_cast<T*>(reinterpret_cast<u32>(memory) + BLOCK_HEADER_SIZE);
    for (size_t i = 0; i < count; i++) {
        new (&ptr[i]) T();
    }
    return ptr;
}


template <typename T>
void dealloc(T* ptr) {
    if (ptr == nullptr) return;
    u32* blockHeader = (u32*)ptr - 2;
    const u32 heapType = blockHeader[0];
    const u32 count = blockHeader[1];
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (size_t i = 0; i < count; i++) {
            ptr[i].~T();
        }
    }
    if (heapType == HEAP_PLUGIN)
        std::free(blockHeader);
    else
        gstd_free(blockHeader);
}

namespace UniqueAlloc {
    struct CustomDestructor {
        template <typename T>
        void operator()(T* ptr) const {
            dealloc<T>(ptr);
        }
    };

    template <typename T, typename... Args>
    std::unique_ptr<T, CustomDestructor> alloc(Args&&... args) {
        return std::unique_ptr<T, CustomDestructor>(Core::alloc<T>(std::forward<Args>(args)...));
    }

    template <typename T, typename... Args>
    std::unique_ptr<T, CustomDestructor> alloc_array(Args&&... args) {
        return std::unique_ptr<T, CustomDestructor>(Core::alloc_array<T>(std::forward<Args>(args)...));
    }

    template <typename T, typename... Args>
    std::unique_ptr<T, CustomDestructor> alloc_raw(Args&&... args) {
        return std::unique_ptr<T, CustomDestructor>(Core::alloc_raw<T>(std::forward<Args>(args)...));
    }

    template <typename T, typename... Args>
    std::unique_ptr<T, CustomDestructor> alloc_array_raw(Args&&... args) {
        return std::unique_ptr<T, CustomDestructor>(Core::alloc_array_raw<T>(std::forward<Args>(args)...));
    }
}

template <typename T>
using unique_ptr = std::unique_ptr<T, UniqueAlloc::CustomDestructor>;

}