#pragma once

#include <3ds.h>

class CustomMutex
{
    private:
        LightLock _lock;
    public:
        CustomMutex() {
            LightLock_Init(&_lock);
        }

        void lock() {
            LightLock_Lock(&_lock);
        }

        // true on success
        bool try_lock() {
            return LightLock_TryLock(&_lock) == 0;
        }

        void unlock() {
            LightLock_Unlock(&_lock);
        }
};