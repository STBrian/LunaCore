#pragma once

#include <CTRPluginFramework.hpp>
#include <source_location>

namespace Core {
    class CrashHandler {
        public:
            static bool abort;
            static bool coreAbort;
            static const char* coreAbortMsg;

            enum PluginState {
                PLUGIN_PATCHPROCESS = 0,
                PLUGIN_MAIN,
                PLUGIN_MAINLOOP,
                PLUGIN_EXIT
            };
            enum CoreState {
                CORE_INIT = 0,
                CORE_LOADING_RUNTIME,
                CORE_LOADING_SCRIPTS,
                CORE_LOADING_MODS,
                CORE_EVENT,
                CORE_HOOK,
                CORE_HOOKING,
            };
            enum GameState {
                GAME_LOADING = 0,
                GAME_MENU,
                GAME_WORLD,
            };

            static PluginState plg_state;
            static CoreState core_state;
            static GameState game_state;

            static void ReserveMemory();

            static void Abort(const char* errMsg, const std::source_location& location = std::source_location::current());

            __attribute__((noreturn)) static void OnAbort();

            static CTRPluginFramework::Process::ExceptionCallbackState ExceptionCallback(ERRF_ExceptionInfo *excep, CpuRegisters *regs);
    };
}