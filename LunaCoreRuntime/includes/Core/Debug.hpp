#pragma once

#if __STDC_HOSTED__
#include <string>
#include <source_location>

#include "Core/CrashHandler.hpp"
#endif

#include "CoreGlobals.hpp"
#include "lua_common.h"

#ifdef DEBUG
#define LCLOGDEBUG(fmt, ...) \
    Core::Debug::LogRawf("[DEBUG] " fmt ". [%s:%d]\n", ##__VA_ARGS__, __FILE__, __LINE__)
#else
#define LCLOGDEBUG(fmt, ...) 
#endif

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
    #if __STDC_HOSTED__
    inline void Abort(const char* errMsg, const std::source_location& location = std::source_location::current()) {
        CrashHandler::Abort(errMsg, location);
    }
    #endif

    namespace Debug {
        #if __STDC_HOSTED__
        void ReportInternalError(const std::string& msg, const std::source_location& location = std::source_location::current());

        bool OpenLogFile(const std::string& filepath);
        #endif

        void CloseLogFile();

        void LogRawf(const char* fmt, ...);

        /* Logs to file */
        void LogInfof(const char* fmt, ...);

        void LogErrorf(const char* fmt, ...);

        void LogWarnf(const char* fmt, ...);

        /* Prints the message on screen and writes to log file */
        void Message(const char* msg);

        #if __STDC_HOSTED__
        void LogRaw(const std::string& msg);

        /* Logs to file */
        void LogInfo(const std::string& msg);

        void LogError(const std::string& msg);

        void LogWarn(const std::string& msg);

        /* Prints the message on screen and writes to log file */
        void Message(const std::string& msg);
        #endif
    }
}