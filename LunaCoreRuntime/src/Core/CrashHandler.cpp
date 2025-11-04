#include "Core/CrashHandler.hpp"

#include <cstdlib>

#include <FsLib/fslib.hpp>
#include "Core/Debug.hpp"
#include "CoreGlobals.hpp"

namespace CTRPF = CTRPluginFramework;

static void *reservedMemory = nullptr;

namespace Core {
    bool CrashHandler::abort = false;
    CrashHandler::PluginState CrashHandler::plg_state = CrashHandler::PLUGIN_PATCHPROCESS;
    CrashHandler::CoreState CrashHandler::core_state = CrashHandler::CORE_INIT;
    CrashHandler::GameState CrashHandler::game_state = CrashHandler::GAME_LOADING;

    void CrashHandler::ReserveMemory() {
        reservedMemory = malloc(4 * 1024);
    }

    void CrashHandler::OnAbort() {
        abort = true;
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
            {
                Core::Debug::LogError("[CRITICAL] Game crashed due to an unhandled error");
                u8 rnd = (svcGetSystemTick() & 0xF00) >> 8;
                Core::Debug::LogRaw(CTRPF::Utils::Format("\t\"%s\"\n", errorMsg[rnd]));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\t\tPlugin state: %d\n", Core::CrashHandler::plg_state));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\t\tLast Core state: %d\n", Core::CrashHandler::core_state));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\t\tGame state: %d\n", Core::CrashHandler::game_state));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\n\tException type: %X\n", excep->type));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tException at address: %08X\n", regs->pc));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tR0: %08X\tR1: %08X\n", regs->r[0], regs->r[1]));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tR2: %08X\tR3: %08X\n", regs->r[2], regs->r[3]));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tR4: %08X\tR5: %08X\n", regs->r[4], regs->r[5]));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tR6: %08X\tR7: %08X\n", regs->r[6], regs->r[7]));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tR8: %08X\tR9: %08X\n", regs->r[8], regs->r[9]));
            }
            u8 possibleError = 0;
            if (plg_state == PluginState::PLUGIN_PATCHPROCESS) possibleError = 1;
            else if (possibleOOM) possibleError = 2;
            else if (luaEnvBusy) possibleError = 3;
            else if (plg_state == PluginState::PLUGIN_MAIN) possibleError = 4;
            u32 pc = regs->pc - 0x100000 < 0x919000 ? regs->pc - 0x100000 : 0;
            pc = (pc >> 2) & 0b1111111111111111111111;
            u32 errorCode = (excep->type << 30 | core_state << 27 | game_state << 25 | possibleError << 22 | pc);
            Core::Debug::LogRaw(CTRPF::Utils::Format("\tError code: %08X\n", errorCode));
            Core::Debug::CloseLogFile();
            if (plg_state != PluginState::PLUGIN_MAINLOOP)
                return CTRPF::Process::EXCB_REBOOT;
            else {
                fslib::closeDevice(u"extdata");
                fslib::exit();
            }

            CTRPF::Screen topScreen = CTRPF::OSD::GetTopScreen();
            topScreen.DrawRect(20, 20, 360, 200, CTRPF::Color::Black, true);
            const char *titleMsg = "Oops.. Game crashed!";
            const char *tipMsg = "See the log file for more details about the crash";
            const char *tipMsg2 = "Press A to exit or B to continue";
            if (abort)
                titleMsg = "Oh no... Game abort!";
            if (possibleOOM)
                titleMsg = "Looks like we ran out of memory!";
            if (regs->pc == 0x00114A98)
                titleMsg = "Game crashed! The game wanted to say something";
            topScreen.DrawSysfont(titleMsg, 25, 25, CTRPF::Color::Red);
            topScreen.DrawSysfont(tipMsg, 25, 40, CTRPF::Color::White);
            topScreen.DrawSysfont(tipMsg2, 25, 55, CTRPF::Color::White);
            topScreen.DrawSysfont(CTRPF::Utils::Format("Error code: %08X", errorCode), 25, 70, CTRPF::Color::White);
            CTRPF::OSD::SwapBuffers();
        }
        CTRPF::Controller::Update();
        if (CTRPF::Controller::IsKeyDown(CTRPF::Key::A)) return CTRPF::Process::EXCB_RETURN_HOME;
        if (CTRPF::Controller::IsKeyDown(CTRPF::Key::B)) return CTRPF::Process::EXCB_DEFAULT_HANDLER;
        else return CTRPF::Process::EXCB_LOOP;
    }
}