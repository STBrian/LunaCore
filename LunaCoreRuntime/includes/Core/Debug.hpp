#pragma once

#if __STDC_HOSTED__
#include <string>
#include <source_location>
#endif

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
    namespace Debug {
        #if __STDC_HOSTED__
        void ReportInternalError(const std::string& msg, const std::source_location& location = std::source_location::current());

        bool OpenLogFile(const std::string& filepath);
        #endif

        void CloseLogFile();

        #if __STDC_HOSTED__
        void LogRaw(const std::string& msg);
        #endif

        void LogRawf(const char* fmt, ...);

        #if __STDC_HOSTED__
        void LogInfo(const std::string& msg);
        #endif

        void LogInfof(const char* fmt, ...);

        #if __STDC_HOSTED__
        void LogError(const std::string& msg);
        #endif

        void LogErrorf(const char* fmt, ...);

        #if __STDC_HOSTED__
        void LogWarn(const std::string& msg);

        void Message(const std::string& msg);
        #endif
    }

    namespace Module {
        bool RegisterDebugModule(lua_State *L);
    }
}