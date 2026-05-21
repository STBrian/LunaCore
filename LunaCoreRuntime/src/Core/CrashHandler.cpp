#include "Core/CrashHandler.hpp"

#include <cstdlib>

#include <FsLib/fslib.hpp>
#include "Core/Debug.hpp"
#include "CoreGlobals.hpp"
#include "ExtendedHeap.hpp"

namespace CTRPF = CTRPluginFramework;

static void *reservedMemory = nullptr;

extern "C" void PLGLDR__DisplayErrMessage(const char*, const char*, u32);

static bool internalAbort = false;
static void* internalAbortLr = nullptr;

extern "C" void __wrap_abort() {
    internalAbortLr = __builtin_return_address(0);
    if (!reservedMemory || internalAbort) {
        PLGLDR__DisplayErrMessage("Critical exception", "An unhandled critical exception ocurred", 0);
        //CTRPF::System::Reboot();
        *(u32*)nullptr = 0; // Double call. Make it crash (so the exception handler can throw the old exception)
        for (;;);
    }
    internalAbort = true;
    Core::CrashHandler::ExceptionCallback(nullptr, nullptr);
    for (;;);
}

static const char* exceptionStr [] = {
    "Prefetch abort",
    "Data abort",
    "Undefined instruction",
    "VFP (floating point) exception"
};

static const char* pluginStateStr[] = {
    "Patch process",
    "Main",
    "Mainloop",
    "Exit"
};

static const char* coreStateStr[] = {
    "Core startup",
    "Loading Lua runtime",
    "Loading scripts",
    "Loading mods",
    "Event triggered",
    "Executing a hook in a game function",
    "Setting up hooks"
};

static const char* gameStateStr[] = {
    "Loading",
    "Menu",
    "World"
};

namespace Core {
    static bool coreAbort = false;
    static void* coreAbortLr = nullptr;
    static const char* coreAbortMsg = nullptr;

    CrashHandler::PluginState CrashHandler::plg_state = CrashHandler::PLUGIN_PATCHPROCESS;
    CrashHandler::CoreState CrashHandler::core_state = CrashHandler::CORE_INIT;
    CrashHandler::GameState CrashHandler::game_state = CrashHandler::GAME_LOADING;
    CrashHandler::CrashHandlerCallback CrashHandler::callback = nullptr;

    void CrashHandler::Init() {
        CTRPF::Process::exceptionCallback = CrashHandler::ExceptionCallback;
        #ifdef DEBUG
        CTRPF::Process::ThrowOldExceptionOnCallbackException = false;
        #else
        CTRPF::Process::ThrowOldExceptionOnCallbackException = true;
        #endif
        reservedMemory = malloc(4 * 1024);
    }

    void CrashHandler::Abort(const char* errMsg, const std::source_location& location) {
        coreAbortLr = __builtin_return_address(0);
        if (coreAbort) {
            PLGLDR__DisplayErrMessage("Critical exception", "An unhandled critical exception ocurred", 0);
            //CTRPF::System::Reboot();
            *(u32*)nullptr = 0; // Double call. Make it crash (so the exception handler can throw the old exception)
        }
        coreAbort = true;
        coreAbortMsg = errMsg;
        if (reservedMemory) { // Free reserved memory cause ReportInternalError may need that memory if OOM
            free(reservedMemory);
            reservedMemory = nullptr;
        }
        Debug::ReportInternalError(errMsg, location);
        CrashHandler::ExceptionCallback(nullptr, nullptr);
        for (;;);
    }

    static const char* ErrorCodeToString(ErrorCode errCode) {
        switch (errCode) {
        case ErrorCode::None:
            return "None";
        
        case ErrorCode::Allocation_Error:
            return "Allocation error";

        default:
            return "Unknown";
        }
    }

    void CrashHandler::Abort(ErrorCode errCode) {
        coreAbortLr = __builtin_return_address(0);
        if (coreAbort) {
            PLGLDR__DisplayErrMessage("Critical exception", "An unhandled critical exception ocurred", 0);
            //CTRPF::System::Reboot();
            *(u32*)nullptr = 0; // Double call. Make it crash (so the exception handler can throw the old exception)
        }
        coreAbort = true;
        coreAbortMsg = ErrorCodeToString(errCode);
        CrashHandler::ExceptionCallback(nullptr, nullptr);
        for (;;);
    }

    static const char *errorMsg[] = {
        "The game had no desire",
        "It was inevitable",
        "Got distracted for a second...",
        "Stay calm and call a trusted adult",
        "Don't panic!",
        "Always expect the unexpected",
        "Who tried sqrt(-1)?",
        "I guess you are not happy :(",
        "Well... that wasn't planned",
        "[Insert silly comment]",
        "Weird, it works for me",
        "Someone should fix this",
        "You discovered an easter egg :D!",
        "  (O_O\"\\)  ",
        "I'm sorry :(",
        "You didn't expect this!"
    };

    CTRPF::Process::ExceptionCallbackState CrashHandler::ExceptionCallback(ERRF_ExceptionInfo *excep, CpuRegisters *regs) {
        if (callback != nullptr) {
            const char* stateMsg = callback(excep, regs);
            if (stateMsg != nullptr) {
                coreAbort = true;
                coreAbortMsg = stateMsg;
            }
        }
        const bool manuallyCalled = excep == nullptr || regs == nullptr;
        const bool gamesFault = manuallyCalled ? false : (regs->pc >= 0x100000 && regs->pc < 0xa19000);
        const bool luaRuntimeFault = !Lua_Global_Mut.try_lock();
        
        if (reservedMemory) {
            free(reservedMemory);
            reservedMemory = nullptr;
        }
        
        Lua_Global_Mut.unlock();
        if (Lua_global != NULL) {
            lua_close(Lua_global);
            Lua_global = nullptr;
        }
        
        // Write exception info to the log file
        if (Core::Debug::LogFileIsOpen()) {
            if (manuallyCalled)
                Core::Debug::LogErrorf("[CRITICAL] LunaCore aborted due to an unhandled error");
            else {
                if (gamesFault)
                    Core::Debug::LogErrorf("[CRITICAL] Game crashed due to an unhandled error");
                else
                    Core::Debug::LogErrorf("[CRITICAL] LunaCore crashed due to an unhandled error");
            }
            
            u8 rnd = (svcGetSystemTick() & 0xF00) >> 8;
            Core::Debug::LogRawf("\t\"%s\"\n", errorMsg[rnd]);
            Core::Debug::LogRawf("\t\tPlugin state: %s\n", pluginStateStr[plg_state]);
            Core::Debug::LogRawf("\t\tLast Core state: %s\n", coreStateStr[core_state]);
            Core::Debug::LogRawf("\t\tGame state: %s\n", gameStateStr[game_state]);
            if (!manuallyCalled) {
                Core::Debug::LogRawf("\n\tException type: %s\n", exceptionStr[excep->type]);
                Core::Debug::LogRawf("\tException at address: %08X\n", regs->pc);
                Core::Debug::LogRawf("\tLR: %08X\n", regs->lr);
                Core::Debug::LogRawf("\tSP: %08X\n", regs->sp);
                Core::Debug::LogRawf("\tCPSR: %08X\n", regs->cpsr);
                Core::Debug::LogRawf("\tR0: %08X \tR1: %08X\n", regs->r[0], regs->r[1]);
                Core::Debug::LogRawf("\tR2: %08X \tR3: %08X\n", regs->r[2], regs->r[3]);
                Core::Debug::LogRawf("\tR4: %08X \tR5: %08X\n", regs->r[4], regs->r[5]);
                Core::Debug::LogRawf("\tR6: %08X \tR7: %08X\n", regs->r[6], regs->r[7]);
                Core::Debug::LogRawf("\tR8: %08X \tR9: %08X\n", regs->r[8], regs->r[9]);
                Core::Debug::LogRawf("\tR10: %08X\tR11: %08X\n", regs->r[10], regs->r[11]);
                Core::Debug::LogRawf("\tR12: %08X\n", regs->r[12]);
            } else {
                Core::Debug::LogRawf("\n\tException type: Core abort\n");
                if (coreAbort) {
                    Core::Debug::LogRawf("\tAbort message:\n");
                    Core::Debug::LogRawf("\t\t%s\n", coreAbortMsg);
                } else {
                    Core::Debug::LogRawf("\n\tException type: Core abort\n");
                    Core::Debug::LogRawf("\tAn unhandled core abort ocurred at %08X\n", (u32)internalAbortLr);
                }
            }
        }
        
        // Make the error code
        const u32 pcOriginal = !manuallyCalled ? regs->pc : ((u32)(coreAbortLr ? coreAbortLr : internalAbortLr));
        const u32 pc = ((gamesFault ? pcOriginal - 0x00100000 : pcOriginal - 0x07000100) >> 2) & 0b1111111111111111111111;
        const ERRF_ExceptionType errtype = !manuallyCalled ? excep->type : ERRF_EXCEPTION_UNDEFINED;
        const u8 errFmt = 3;

        const u32 errorCode = (errtype << 30 | core_state << 27 | game_state << 25 | errFmt << 23 | !gamesFault << 22 | pc);
        if (Core::Debug::LogFileIsOpen()) {
            Core::Debug::LogRawf("\tError code: %08X\n", errorCode);
            Core::Debug::CloseLogFile();
        }
        if (plg_state != PluginState::PLUGIN_PATCHPROCESS) {
            fslib::close_device(u"extdata");
            fslib::exit();
        }

        const char* errtitle = "Oops... Game crashed!";
        char errcontent[0x100];
        char extrainfobuffer[0x80];
        const char* extrainfo = coreAbortMsg ? coreAbortMsg : "";
        const char* actionmsg = plg_state == PluginState::PLUGIN_MAINLOOP ? "The game will exit now" : "The console will reboot now";
        if (coreAbortMsg != nullptr) extrainfo = coreAbortMsg;
        if (regs == nullptr || excep == nullptr) {
            errtitle = "Fatal core exception";
        } else {
            if (!gamesFault) errtitle = "Oh no... Things got out of hand!";

            if (pcOriginal == 0x00114A98) errtitle = "Game assertion exception";

            if (excep->type == ERRF_ExceptionType::ERRF_EXCEPTION_DATA_ABORT) {
                snprintf(extrainfobuffer, sizeof(extrainfobuffer), "%s\nAccess at: 0x%08X", extrainfo, excep->far);
                extrainfo = extrainfobuffer;
            }
        }
        snprintf(errcontent, sizeof(errcontent), "An unhandled exception ocurred.\n%s\n%s", extrainfo, actionmsg);
        PLGLDR__DisplayErrMessage(errtitle, errcontent, errorCode);
        if (plg_state != PluginState::PLUGIN_MAINLOOP)
            CTRPF::System::Reboot();
        else
            CTRPF::Process::ReturnToHomeMenu();
        return CTRPF::Process::EXCB_LOOP;
    }
}