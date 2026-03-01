#pragma once

#include <stdlib.h>
#include <new>
#include <utility>
#include <memory>
#include <type_traits>

#include "Helpers/Errors.hpp"
#include "Core/CrashHandler.hpp"

namespace Core {

template <typename T, typename... Args>
T* alloc(Args&&... args) {
    void* ptr = std::malloc(sizeof(T));
    if (ptr == nullptr) CrashHandler::Abort(ErrorCode::Allocation_Error);
    return new (ptr) T(std::forward<Args>(args)...);
}

template <typename T>
T* alloc_array(size_t count) {
    void* memory = std::malloc(sizeof(T) * count + 0x8);
    if (memory == nullptr) CrashHandler::Abort(ErrorCode::Allocation_Error);

    *reinterpret_cast<u32*>(memory) = count;
    T* ptr = reinterpret_cast<T*>(reinterpret_cast<u32>(memory) + 0x8);

    for (size_t i = 0; i < count; i++) {
        new (&ptr[i]) T();
    }
    return ptr;
}

template <typename T, typename... Args>
T* alloc_raw(Args&&... args) {
    void* ptr = std::malloc(sizeof(T));
    if (ptr == nullptr) return nullptr;
    return new (ptr) T(std::forward<Args>(args)...);
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
    std::free(ptr);
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