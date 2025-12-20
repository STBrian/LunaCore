#pragma once

#include "types.h"

#define IN_HEAP(p) ((u32)p > 0x30000000 && (u32)p < 0x40000000)
#define IS_GAME_ALLOCATED(p) (*((u32*)p - 4) == 0x5544)
#define CHECK_GAME_POINTER(p) (IN_HEAP(p) && IS_GAME_ALLOCATED(p))

namespace gstd {

/*
Class intended to validate game pointers to allocated memory blocks with the game's new operator or gstd::malloc function
*/
template<typename T>
class alloc_ptr {
    private:
    T* ptr;

    public:
    alloc_ptr(T* p) {
        ptr = p;
    }

    /*
    Checks if the pointer is in heap and contains magic value
    */
    bool check() {
        return CHECK_GAME_POINTER(ptr);
    }

    /*
    If valid, returns the allocated memory block size, 0 otherwise
    */
    size_t getSize() {
        if (check())
            return *((u32*)ptr - 3);
        return 0;
    }

    /*
    Returns nullptr if the pointer is invalid
    */
    T* get() {
        if (check()) 
            return ptr;
        return nullptr;
    }

    explicit operator bool() const {
        return CHECK_GAME_POINTER(ptr);
    }

    T* operator+(int o) const {
        return this->ptr + o;
    }

    T& operator*() {
        return *ptr;
    }
};
}