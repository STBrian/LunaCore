#include "Core/Debug.hpp"

#include <string>
#include <time.h>
#include <CTRPluginFramework.hpp>

#include "Core/Utils/Utils.hpp"

#include "stdarg.h"

namespace CTRPF = CTRPluginFramework;

static CTRPF::File logFile;

void Core::Debug::ReportInternalError(const std::string& msg, const std::source_location& location) {
    Core::Debug::LogError("Internal core exception! \n\tAt function: " + 
        std::string(location.function_name()) + 
        "\n\tat line: " + std::to_string(location.line()) +
        "\n\tError message: " + msg
    );
}

bool Core::Debug::OpenLogFile(const std::string& filepath)
{
    if (!CTRPF::File::Exists(filepath))
        CTRPF::File::Create(filepath);
    CTRPF::File::Open(logFile, filepath);
    if (logFile.IsOpen())
        logFile.Clear();
    return logFile.IsOpen();
}

void Core::Debug::CloseLogFile()
{
    if (logFile.IsOpen())
        logFile.Close();
}

void Core::Debug::LogRaw(const std::string& msg)
{
    if (logFile.IsOpen()) {
        std::string newMsg(msg);
        Core::Utils::Replace(newMsg, "\t", "    ");
        logFile.Write(newMsg.c_str(), newMsg.size());
        logFile.Flush();
    }
}

void Core::Debug::LogRawf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int size = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);

    va_start(args, fmt);

    char* buffer = (char*)malloc(size);
    if (buffer) {
        vsnprintf(buffer, size, fmt, args);
        Core::Debug::LogRaw(buffer);
        free(buffer);
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

    char* buffer = (char*)malloc(size);
    if (buffer) {
        vsnprintf(buffer, size, fmt, args);
        DebugWriteLog_impl(buffer);
        free(buffer);
    }

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

    char* buffer = (char*)malloc(size);
    if (buffer) {
        vsnprintf(buffer, size, fmt, args);
        Core::Debug::LogError(buffer);
        free(buffer);
    }

    va_end(args);
}

void Core::Debug::LogWarnf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int size = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);

    va_start(args, fmt);

    char* buffer = (char*)malloc(size);
    if (buffer) {
        vsnprintf(buffer, size, fmt, args);
        Core::Debug::LogWarn(buffer);
        free(buffer);
    }

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