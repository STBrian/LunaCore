#pragma once

#include <3ds.h>

namespace Core {
class Mutex
{
    private:
        LightLock _lock;
    public:
        Mutex() {
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
}