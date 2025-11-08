#include "Helpers/Threads.hpp"

namespace Core {
void Thread::threadBodyFunction(threadCtx* ctx) {
    ctx->callback((void*)ctx->passValue);
    u8* stackStart = ctx->stackPtr;
    delete ctx;
    free(stackStart);
    svcExitThread();
}

Thread::Thread(ThreadEmptyFunc func) {
    u8* stackPtr = (u8*)memalign(0x8, 0x800);
    if (!stackPtr)
        return;
    threadCtx* ctx = new threadCtx;
    ctx->stackPtr = stackPtr;
    ctx->stackPtrTop = stackPtr + 0x800;
    ctx->callback = (ThreadFunc)func;
    ctx->passValue = 0;
    svcCreateThread(&this->threadHndl, (ThreadFunc)threadBodyFunction, (u32)ctx, (u32*)ctx->stackPtrTop, 0x30, -2);
    this->started = true;
}
}