#pragma once

#include <CTRPluginFramework.hpp>
#include <source_location>

#include "Helpers/Errors.hpp"

namespace Core {
    class CrashHandler {
        public:
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
            
            using CrashHandlerCallback = const char* (*)(ERRF_ExceptionInfo*, CpuRegisters*);
            static CrashHandlerCallback callback;

            static void Init();

            /* Callback should return nullptr to indicate success. Otherwise, return
            an error message */
            static void SetCallback(CrashHandlerCallback fun) {
                callback = fun;
            }

            NORETURN static void Abort(const char* errMsg, const std::source_location& location = std::source_location::current());

            NORETURN static void Abort(ErrorCode errCode);

            static CTRPluginFramework::Process::ExceptionCallbackState ExceptionCallback(ERRF_ExceptionInfo *excep, CpuRegisters *regs);
    };
}