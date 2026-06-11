#pragma once

#if __STDC_HOSTED__
#include <source_location>
#include "Core/CrashHandler.hpp"
#endif

#include <string_view>

#include "CoreGlobals.hpp"
#include "lua_common.h"

#ifdef DEBUG
#pragma message "Debug enabled"
#define LOGDEBUG(fmt, ...) \
    Core::Debug::LogInfof("[DEBUG] " fmt " [%s:%d]", ##__VA_ARGS__, __FILE__, __LINE__)
#else
#define LOGDEBUG(fmt, ...) 
#endif

#ifndef ASSERT
#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            Core::Debug::LogErrorf("ASSERT failed: " #cond "(" __FILE__ ": %d)", __LINE__); \
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

        bool LogFileIsOpen();

        void LogRawf(const char* fmt, ...);

        /* Logs to file */
        void LogInfof(const char* fmt, ...);

        void LogErrorf(const char* fmt, ...);

        void LogWarnf(const char* fmt, ...);

        /* Prints the message on screen and writes to log file */
        void Message(const char* msg);

        void LogRaw(std::string_view msg);

        /* Logs to file */
        void LogInfo(std::string_view msg);

        void LogError(std::string_view msg);

        void LogWarn(std::string_view msg);

        /* Prints the message on screen and writes to log file */
        void Message(std::string_view msg);
    }
}