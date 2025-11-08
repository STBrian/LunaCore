#pragma once

#include <concepts>
#include <type_traits>

#include <malloc.h>
#include "3ds.h"

namespace Core {
    template<typename F, typename T>
    concept VoidFunctionOneArg = requires(F f, T t) {
        { f(t) } -> std::same_as<void>;
    };

    template<typename T>
    concept PrimitiveOrPointer = std::is_arithmetic_v<T> || std::is_pointer_v<T>;

    template<typename T>
    concept PrimitiveIntegerOrPointerNot64 = (std::is_integral_v<T> && sizeof(T) < 8) || std::is_pointer_v<T>;

    /* The thread will start automatically when the instance is created */
    class Thread {
        private:
        Handle threadHndl;
        bool started = false;

        typedef struct {
            u8* stackPtr;
            u8* stackPtrTop;
            ThreadFunc callback;
            u32 passValue;
        } threadCtx;
        
        private:
        Thread() {}

        static void threadBodyFunction(threadCtx* ctx);

        using ThreadEmptyFunc = void (*)(void);

        public:
        Thread(ThreadEmptyFunc func);

        template<typename F, typename T>
        requires VoidFunctionOneArg<F, T> && PrimitiveIntegerOrPointerNot64<T>
        Thread(F func, T value) {
            u8* stackPtr = (u8*)memalign(0x8, 0x800);
            if (!stackPtr)
                return;
            threadCtx* ctx = new threadCtx;
            ctx->stackPtr = stackPtr;
            ctx->stackPtrTop = stackPtr + 0x800;
            ctx->callback = (ThreadFunc)func;
            ctx->passValue = reinterpret_cast<u32>(value);
            svcCreateThread(&this->threadHndl, (ThreadFunc)threadBodyFunction, (u32)ctx, (u32*)ctx->stackPtrTop, 0x30, -2);
            this->started = true;
        }

        bool isStarted() {
            return started;
        }

        class Sleep {
            static void Nanoseconds(u64 ns) {
                svcSleepThread(ns);
            }

            static void Milliseconds(u32 ms) {
                svcSleepThread(ms * 1000000LL);
            }

            static void Seconds(u32 sec) {
                svcSleepThread(sec * 1000000000LL);
            }
        };
    };
}