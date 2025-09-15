#pragma once

#include <string>

#include "CoreGlobals.hpp"
#include "lua_common.h"

#ifndef ASSERT
#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            Core::Debug::LogError(std::string("ASSERT failed: ") + #cond + "(" + __FILE__ + ": " + std::to_string(__LINE__) + ")"); \
            abort(); \
        } \
    } while (0)
#endif

namespace Core {
    namespace Debug {
        extern const char *tab;
        
        bool OpenLogFile(const std::string& filepath);

        void CloseLogFile();

        void LogRaw(const std::string& msg);

        void LogMessage(const std::string& msg, bool showOnScreen);

        void LogMessage(const char* msg, bool showOnScreen);

        void LogError(const std::string& msg);

        void LogError(const char* msg);

        void Message(const std::string& msg);

        void Message(const char* msg);

        void Error(const std::string& msg);

        void Error(const char* msg);
    }

    namespace Module {
        bool RegisterDebugModule(lua_State *L);
    }
}