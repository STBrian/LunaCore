#include "Core/CrashHandler.hpp"

#include <cstdlib>

#include <FsLib/fslib.hpp>
#include "Core/Debug.hpp"
#include "CoreGlobals.hpp"

namespace CTRPF = CTRPluginFramework;

static void *reservedMemory = nullptr;

extern "C" void PLGLDR__DisplayErrMessage(const char*, const char*, u32);
extern "C" void __real_abort(void);

static bool internalAbort = false;
static void* internalAbortLr = nullptr;

extern "C" void __wrap_abort() {
    if (!reservedMemory)
        __real_abort();
    internalAbort = true;
    internalAbortLr = __builtin_return_address(0);
    *(u32*)nullptr = 0;
    for (;;);
}

namespace Core {
    static bool coreAbort = false;
    static const char* coreAbortMsg = nullptr;

    CrashHandler::PluginState CrashHandler::plg_state = CrashHandler::PLUGIN_PATCHPROCESS;
    CrashHandler::CoreState CrashHandler::core_state = CrashHandler::CORE_INIT;
    CrashHandler::GameState CrashHandler::game_state = CrashHandler::GAME_LOADING;

    void CrashHandler::ReserveMemory() {
        reservedMemory = malloc(4 * 1024);
    }

    void CrashHandler::Abort(const char* errMsg, const std::source_location& location) {
        coreAbort = true;
        coreAbortMsg = errMsg;
        Debug::ReportInternalError(errMsg, location);
        *(u32*)nullptr = 0;
        for (;;);
    }

    static const char *errorMsg[] = {
        "The game had no desire",
        "It was inevitable",
        "Got distracted for a second and...",
        "Stay calm and call a trusted adult",
        "Don't panic!",
        "Always expect the unexpected",
        "Who tried sqrt(-1)?",
        "I guess you are not happy :(",
        "Well... that wasn't planned",
        "[Insert silly comment]",
        "Weird, it works for me",
        "Someone should fix this",
        "Two guys are walking, and the game falls",
        "  (O_O\"\\)  ",
        "I'm sorry :(",
        "You didn't expect this!"
    };

    CTRPF::Process::ExceptionCallbackState CrashHandler::ExceptionCallback(ERRF_ExceptionInfo *excep, CpuRegisters *regs) {
        static bool first = true;
        if (first) {
            first = false;
            if (reservedMemory) {
                free(reservedMemory);
                reservedMemory = nullptr;
            }
            bool possibleOOM = false;
            bool luaEnvBusy = !Lua_Global_Mut.try_lock();
            Lua_Global_Mut.unlock();
            if (Lua_global != NULL) {
                if (!luaEnvBusy)
                    possibleOOM = lua_gc(Lua_global, LUA_GCCOUNT, 0) >= 2000;
                lua_close(Lua_global);
            }
            
            if (coreAbort || internalAbort)
                Core::Debug::LogInfof("[CRITICAL] LunaCore aborted due to an unhandled error");
            else
                Core::Debug::LogInfof("[CRITICAL] Game crashed due to an unhandled error");
            
            u8 rnd = (svcGetSystemTick() & 0xF00) >> 8;
            Core::Debug::LogRawf("\t\"%s\"\n", errorMsg[rnd]);
            Core::Debug::LogRawf("\t\tPlugin state: %d\n", Core::CrashHandler::plg_state);
            Core::Debug::LogRawf("\t\tLast Core state: %d\n", Core::CrashHandler::core_state);
            Core::Debug::LogRawf("\t\tGame state: %d\n", Core::CrashHandler::game_state);
            if (coreAbort) {
                Core::Debug::LogRawf("\n\tException type: Core abort\n");
                Core::Debug::LogRawf("\tAbort message:\n");
                Core::Debug::LogRawf("\t\t%s\n", coreAbortMsg);
            } else if (internalAbort) {
                Core::Debug::LogRawf("\n\tException type: Core abort\n");
                Core::Debug::LogRawf("\tAn unhandled core abort ocurred at %08X\n", (u32)internalAbortLr);
            } else {
                Core::Debug::LogRawf("\n\tException type: %X\n", excep->type);
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
            }
            
            u8 possibleError = 0;
            if (plg_state == PluginState::PLUGIN_PATCHPROCESS) possibleError = 1;
            else if (luaEnvBusy) possibleError = 2;
            else if (possibleOOM) possibleError = 3;
            u32 pc = 0;
            bool pluginFault = false;
            if (regs->pc - 0x00100000 < 0x919000) {
                pc = regs->pc - 0x00100000;
            } else if (regs->pc < 0x30000000) {
                pc = regs->pc - 0x07000100;
                pluginFault = true;
            }
            pc = (pc >> 2) & 0b1111111111111111111111;
            u32 errorCode = (excep->type << 30 | core_state << 27 | game_state << 25 | possibleError << 23 | pluginFault << 22 | pc);
            Core::Debug::LogRawf("\tError code: %08X\n", errorCode);
            Core::Debug::CloseLogFile();
            if (plg_state != PluginState::PLUGIN_MAINLOOP) {
                if (coreAbortMsg != nullptr)
                    PLGLDR__DisplayErrMessage("Fatal core exception", (std::string(coreAbortMsg) + "\nThe console will reboot now").c_str(), errorCode);
                else 
                    PLGLDR__DisplayErrMessage("Fatal core exception", "An unhandled exception ocurred.\nThe console will reboot now", errorCode);
                return CTRPF::Process::EXCB_REBOOT;
            } else {
                fslib::close_device(u"extdata");
                fslib::exit();
            }

            CTRPF::Screen topScreen = CTRPF::OSD::GetTopScreen();
            topScreen.DrawRect(20, 20, 360, 200, CTRPF::Color::Black, true);
            const char *titleMsg = "Oops.. Game crashed!";
            if (internalAbort || coreAbort)
                titleMsg = "Oh no... Things got out of hand!";

            if (possibleOOM)
                titleMsg = "Game ran out of memory!";
            if (regs->pc == 0x00114A98)
                titleMsg = "Game crashed! An unhandled game assert exception occurred";
            topScreen.DrawSysfont(titleMsg, 25, 25, CTRPF::Color::Red);
            topScreen.DrawSysfont("See the log file for more details about the crash", 25, 40, CTRPF::Color::White);
            topScreen.DrawSysfont("Press A to exit or B to reboot", 25, 55, CTRPF::Color::White);
            topScreen.DrawSysfont(CTRPF::Utils::Format("Error code: %08X", errorCode), 25, 70, CTRPF::Color::White);
            CTRPF::OSD::SwapBuffers();
        }
        CTRPF::Controller::Update();
        if (CTRPF::Controller::IsKeyDown(CTRPF::Key::A)) return CTRPF::Process::EXCB_RETURN_HOME;
        if (CTRPF::Controller::IsKeyDown(CTRPF::Key::B)) return CTRPF::Process::EXCB_REBOOT;
        else return CTRPF::Process::EXCB_LOOP;
    }
}