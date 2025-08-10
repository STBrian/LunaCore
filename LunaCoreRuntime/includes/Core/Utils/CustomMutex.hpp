#pragma once

#include <3ds.h>
#include <atomic>

class CustomMutex
{
    private:
        std::atomic<bool> locked = false;
    public:
        CustomMutex() {}
        ~CustomMutex() {}

        void lock() {
            while (this->locked.load()) {
                svcSleepThread(1000000);
            }
            this->locked.store(true);
        }

        bool try_lock() {
            if (this->locked.load())
                return false;
            else 
                this->locked.store(true);
            return true;
        }

        void unlock() {
            this->locked.store(false);
        }
};

class CustomLockGuard {
    private:
        CustomMutex& m_mutex;
    public:
        CustomLockGuard(CustomMutex& mutex): m_mutex(mutex) {
            m_mutex.lock();
        };

        ~CustomLockGuard() {
            m_mutex.unlock();
        };
};