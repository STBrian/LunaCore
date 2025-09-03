#pragma once

#include <string>

#include "CoreGlobals.hpp"
#include "lua_common.h"

#ifndef ASSERT
#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            Core::Debug::LogError(STRING_CLASS("ASSERT failed: ") + #cond + "(" + __FILE__ + ": " + std::to_string(__LINE__) + ")"); \
            abort(); \
        } \
    } while (0)
#endif

namespace Core {
    namespace Debug {
        extern const char *tab;
        
        bool OpenLogFile(const STRING_CLASS& filepath);

        void CloseLogFile();

        void LogRaw(const STRING_CLASS& msg);

        void LogMessage(const STRING_CLASS& msg, bool showOnScreen);

        void LogMessage(const char* msg, bool showOnScreen);

        void LogError(const STRING_CLASS& msg);

        void LogError(const char* msg);

        void Message(const STRING_CLASS& msg);

        void Message(const char* msg);

        void Error(const STRING_CLASS& msg);

        void Error(const char* msg);
    }

    namespace Module {
        bool RegisterDebugModule(lua_State *L);
    }
}