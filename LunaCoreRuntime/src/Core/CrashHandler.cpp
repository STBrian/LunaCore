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

    static const char *getPluginStateString(CrashHandler::PluginState state) {
        const char *str = "unknown";
        switch (state) {
            case CrashHandler::PLUGIN_PATCHPROCESS:
                str = "Patch process";
                break;
            case CrashHandler::PLUGIN_MAIN:
                str = "Main";
                break;
            case CrashHandler::PLUGIN_MAINLOOP:
                str = "Mainloop";
                break;
            case CrashHandler::PLUGIN_EXIT:
                str = "Exit";
                break;
        }
        return str;
    }

    static const char *getCoreStateString(CrashHandler::CoreState state) {
        const char *str = "unknown";
        switch (state) {
            case CrashHandler::CORE_INIT:
                str = "Core startup";
                break;
            case CrashHandler::CORE_LOADING_RUNTIME:
                str = "Loading Lua runtime";
                break;
            case CrashHandler::CORE_LOADING_MODS:
                str = "Loading mods";
                break;
            case CrashHandler::CORE_LOADING_SCRIPTS:
                str = "Loading scripts";
                break;
            case CrashHandler::CORE_EVENT:
                str = "Event triggered";
                break;
            case CrashHandler::CORE_HOOK:
                str = "Hooking a game function";
                break;
            case CrashHandler::CORE_HOOKING:
                str = "Setting up hooks";
                break;
            case CrashHandler::CORE_EXIT:
                str = "Exiting";
                break;
        }
        return str;
    }

    static const char *getGameStateString(CrashHandler::GameState state) {
        const char *str = "unknown";
        switch (state) {
            case CrashHandler::GAME_LOADING:
                str = "Loading";
                break;
            case CrashHandler::GAME_MENU:
                str = "Menu";
                break;
            case CrashHandler::GAME_WORLD:
                str = "World";
                break;
        }
        return str;
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
        "We learn from our mistakes"
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
                u8 rnd = CTRPF::Utils::Random(0, (sizeof(errorMsg) / sizeof(errorMsg[0])) - 1);
                Core::Debug::LogRaw(CTRPF::Utils::Format("\t\"%s\"\n", errorMsg[rnd]));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\t\tPlugin state: %s\n", getPluginStateString(Core::CrashHandler::plg_state)));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\t\tLast Core state: %s\n", getCoreStateString(Core::CrashHandler::core_state)));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\t\tGame state: %s\n", getGameStateString(Core::CrashHandler::game_state)));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\n\tException type: %X\n", excep->type));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tException at address: %08X\n", regs->pc));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tR0: %08X\tR1: %08X\n", regs->r[0], regs->r[1]));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tR2: %08X\tR3: %08X\n", regs->r[2], regs->r[3]));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tR4: %08X\tR5: %08X\n", regs->r[4], regs->r[5]));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tR6: %08X\tR7: %08X\n", regs->r[6], regs->r[7]));
                Core::Debug::LogRaw(CTRPF::Utils::Format("\tR8: %08X\tR9: %08X\n", regs->r[8], regs->r[9]));
            }
            u32 errorCode = (1 << core_state | 1 << (8 + plg_state) | 1 << (12 + game_state) | possibleOOM << 15 | luaEnvBusy << 16);
            const char* reasonMsg = "Unknown";
            if (possibleOOM)
                reasonMsg = "Out of memory. Lua memory was bigger than 2 MB";
            else if (luaEnvBusy)
                reasonMsg = "Core lua runtime error";
            else if (plg_state == PluginState::PLUGIN_PATCHPROCESS)
                reasonMsg = "Core internal error during startup";
            else if (plg_state == PluginState::PLUGIN_MAIN)
                reasonMsg = "Core internal error during initialization";
            Core::Debug::LogRaw(CTRPF::Utils::Format("\tPossible reason: %s\n", reasonMsg));
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
            const char *tipMsg2 = "Press A button to exit or B to continue";
            if (abort)
                titleMsg = "Oh no... Game abort!";
            if (possibleOOM)
                titleMsg = "Looks like we ran out of memory!";
            topScreen.DrawSysfont(titleMsg, 25, 25, CTRPF::Color::Red);
            topScreen.DrawSysfont(tipMsg, 25, 40, CTRPF::Color::White);
            topScreen.DrawSysfont(tipMsg2, 25, 55, CTRPF::Color::White);
            CTRPF::OSD::SwapBuffers();
        }
        CTRPF::Controller::Update();
        if (CTRPF::Controller::IsKeyDown(CTRPF::Key::A)) return CTRPF::Process::EXCB_RETURN_HOME;
        if (CTRPF::Controller::IsKeyDown(CTRPF::Key::B)) return CTRPF::Process::EXCB_DEFAULT_HANDLER;
        else return CTRPF::Process::EXCB_LOOP;
    }
}