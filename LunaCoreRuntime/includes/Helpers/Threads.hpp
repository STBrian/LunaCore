#pragma once

#include <concepts>
#include <type_traits>

#include <3ds.h>

#include "Core/Debug.hpp"
#include "Helpers/Allocation.hpp"

namespace Core {
    template<typename F, typename T>
    concept VoidFunctionOneArg = requires(F f, T t) {
        { f(t) } -> std::same_as<void>;
    };

    template<typename T>
    concept PrimitiveOrPointer = std::is_arithmetic_v<T> || std::is_pointer_v<T>;

    /* The thread will start automatically when the instance is created */
    class Thread {
        private:
        Handle threadHndl;
        bool started = false;

        template<typename T>
        class threadCtx {
            public:
            u8* stackPtr;
            u8* stackPtrTop;
            void (*callback)(T);
            T passValue;
        };
        
        private:
        Thread() {}

        template<typename T>
        static void threadBodyFunction(threadCtx<T>* ctx) {
            ctx->callback(ctx->passValue);
            u8* stackStart = ctx->stackPtr;
            dealloc(ctx);
            dealloc_array(stackStart);
            svcExitThread();
        }

        using ThreadEmptyFunc = void (*)(void);

        public:
        Thread(ThreadEmptyFunc func) {
            auto stackPtr = UniqueAlloc::alloc_array_raw<u8>(0x800);
            auto ctx = UniqueAlloc::alloc_raw<threadCtx<void*>>();
            if (!stackPtr || !ctx) return;
            ctx->stackPtr = stackPtr.release();
            u8* stackPtrTop = ctx->stackPtr + 0x800;
            ctx->stackPtrTop = stackPtrTop;
            ctx->callback = (ThreadFunc)func;
            ctx->passValue = nullptr;
            svcCreateThread(&this->threadHndl, (ThreadFunc)threadBodyFunction<void*>, (u32)ctx.release(), (u32*)stackPtrTop, 0x30, -2);
            this->started = true;
        }

        template<typename F, typename T>
        requires VoidFunctionOneArg<F, T> && PrimitiveOrPointer<T>
        Thread(F func, T value) {
            auto stackPtr = UniqueAlloc::alloc_array_raw<u8>(0x800);
            auto ctx = UniqueAlloc::alloc_raw<threadCtx<T>>();
            if (!stackPtr || !ctx) return;
            ctx->stackPtr = stackPtr.release();
            u8* stackPtrTop = ctx->stackPtr + 0x800;
            ctx->stackPtrTop = stackPtrTop;
            ctx->callback = func;
            ctx->passValue = value;
            svcCreateThread(&this->threadHndl, (ThreadFunc)threadBodyFunction<T>, (u32)ctx.release(), (u32*)stackPtrTop, 0x30, -2);
            this->started = true;
        }

        bool isStarted() {
            return started;
        }

        class Sleep {
            public:
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