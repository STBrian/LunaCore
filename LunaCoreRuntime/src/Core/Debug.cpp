#include "Core/Debug.hpp"

#include <string>
#include <time.h>
#include <CTRPluginFramework.hpp>

#include "Core/Utils/Utils.hpp"

#include "stdarg.h"

namespace CTRPF = CTRPluginFramework;

static Core::File logFile;
Core::Mutex logFileMutex;

void Core::Debug::ReportInternalError(const std::string& msg, const std::source_location& location) {
    Core::Debug::LogError("Internal core exception! \n\tAt function: " + 
        std::string(location.function_name()) + 
        "\n\tat line: " + std::to_string(location.line()) +
        "\n\tError message: " + msg
    );
}

bool Core::Debug::OpenLogFile(const std::string& filepath)
{
    Core::Filesystem::Open(logFile, filepath, FS_OPEN_WRITE|FS_OPEN_CREATE);
    return logFile.isOpen();
}

void Core::Debug::CloseLogFile()
{
    if (logFile.isOpen())
        logFile.close();
}

bool Core::Debug::LogFileIsOpen() {
    return logFile.isOpen();
}

void Core::Debug::LogRaw(const std::string& msg)
{
    if (logFile.isOpen()) {
        logFileMutex.lock();
        std::string newMsg(msg);
        Core::Utils::Replace(newMsg, "\t", "    ");
        logFile.write(newMsg.c_str(), newMsg.size());
        logFile.flush();
        logFileMutex.unlock();
    }
}

void Core::Debug::LogRawf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int size = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);

    va_start(args, fmt);

    char buffer[size];
    vsnprintf(buffer, size, fmt, args);
    if (logFile.isOpen()) {
        logFileMutex.lock();
        logFile.write(buffer, size - 1);
        logFile.flush();
        logFileMutex.unlock();
    }

    va_end(args);
}

static void DebugWriteLog_impl(const std::string& msg)
{
    std::string out_msg = "[";
    out_msg += Core::Utils::formatTime(time(NULL)) + "] " + std::string(msg);
    Core::Debug::LogRaw(out_msg + "\n");
}

void Core::Debug::LogInfo(const std::string& msg) {
    DebugWriteLog_impl(msg);
}

void Core::Debug::LogInfof(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int size = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);

    va_start(args, fmt);

    char buffer[size];
    vsnprintf(buffer, size, fmt, args);
    DebugWriteLog_impl(buffer);

    va_end(args);
}

void Core::Debug::LogError(const std::string& msg) {
    CTRPF::OSD::Notify(msg, CTRPF::Color::Red, CTRPF::Color::Black);
    DebugWriteLog_impl("[ERROR] " + msg);
}

void Core::Debug::LogWarn(const std::string& msg) {
    CTRPF::OSD::Notify(msg, CTRPF::Color::Yellow, CTRPF::Color::Black);
    DebugWriteLog_impl("[WARN] " + msg);
}

void Core::Debug::LogErrorf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int size = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);

    va_start(args, fmt);

    char buffer[size];
    vsnprintf(buffer, size, fmt, args);
    Core::Debug::LogError(buffer);

    va_end(args);
}

void Core::Debug::LogWarnf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int size = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);

    va_start(args, fmt);

    char buffer[size];
    vsnprintf(buffer, size, fmt, args);
    Core::Debug::LogWarn(buffer);

    va_end(args);
}

void Core::Debug::Message(const std::string& msg) {
    CTRPF::OSD::Notify(msg);
    DebugWriteLog_impl(msg);
}

void Core::Debug::Message(const char* msg) {
    CTRPF::OSD::Notify(msg);
    DebugWriteLog_impl(msg);
}