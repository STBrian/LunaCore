#pragma once

#if __STDC_HOSTED__
#include <string>
#include <source_location>
#endif

#include "Core/CrashHandler.hpp"

#include "CoreGlobals.hpp"
#include "lua_common.h"

#ifndef ASSERT
#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            Core::Debug::LogError("ASSERT failed: " #cond "(" __FILE__ ": " __LINE__ ")"); \
            abort(); \
        } \
    } while (0)

#endif

namespace Core {
    inline void Abort(const char* errMsg, const std::source_location& location = std::source_location::current()) {
        CrashHandler::Abort(errMsg, location);
    }

    namespace Debug {
        #if __STDC_HOSTED__
        void ReportInternalError(const std::string& msg, const std::source_location& location = std::source_location::current());

        bool OpenLogFile(const std::string& filepath);
        #endif

        void CloseLogFile();

        void LogRawf(const char* fmt, ...);

        void LogInfof(const char* fmt, ...);

        void LogErrorf(const char* fmt, ...);

        void LogWarnf(const char* fmt, ...);

        void Message(const char* msg);

        #if __STDC_HOSTED__
        void LogRaw(const std::string& msg);

        void LogInfo(const std::string& msg);

        void LogError(const std::string& msg);

        void LogWarn(const std::string& msg);

        void Message(const std::string& msg);
        #endif
    }

    namespace Module {
        bool RegisterDebugModule(lua_State *L);
    }
}