#pragma once

#include <concepts>
#include <type_traits>
#include <new>

#include <malloc.h>
#include <3ds.h>

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
            delete ctx;
            free(stackStart);
            svcExitThread();
        }

        using ThreadEmptyFunc = void (*)(void);

        public:
        Thread(ThreadEmptyFunc func) {
            u8* stackPtr = (u8*)memalign(0x8, 0x800);
            if (!stackPtr)
                return;
            threadCtx<void*>* ctx = new (std::nothrow) threadCtx<void*>;
            if (!ctx) {
                free(stackPtr);
                return;
            }
            ctx->stackPtr = stackPtr;
            ctx->stackPtrTop = stackPtr + 0x800;
            ctx->callback = (ThreadFunc)func;
            ctx->passValue = nullptr;
            svcCreateThread(&this->threadHndl, (ThreadFunc)threadBodyFunction<void*>, (u32)ctx, (u32*)ctx->stackPtrTop, 0x30, -2);
            this->started = true;
        }

        template<typename F, typename T>
        requires VoidFunctionOneArg<F, T> && PrimitiveOrPointer<T>
        Thread(F func, T value) {
            u8* stackPtr = (u8*)memalign(0x8, 0x800);
            if (!stackPtr)
                return;
            threadCtx<T>* ctx = new (std::nothrow) threadCtx<T>;
            if (!ctx) {
                free(stackPtr);
                return;
            }
            ctx->stackPtr = stackPtr;
            ctx->stackPtrTop = stackPtr + 0x800;
            ctx->callback = (ThreadFunc)func;
            ctx->passValue = value;
            svcCreateThread(&this->threadHndl, (ThreadFunc)threadBodyFunction<T>, (u32)ctx, (u32*)ctx->stackPtrTop, 0x30, -2);
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