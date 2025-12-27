#include "Core/Debug.hpp"

#include <string>
#include <time.h>
#include <CTRPluginFramework.hpp>

#include "Core/System.hpp"
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
    out_msg += Core::Utils::formatTime(Core::System::getTime()) + "] " + std::string(msg);
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
        DebugWriteLog_impl(buffer);
        free(buffer);
    }

    va_end(args);
}

void Core::Debug::Message(const std::string& msg) {
    CTRPF::OSD::Notify(msg);
    DebugWriteLog_impl(msg);
}

// ----------------------------------------------------------------------------

//!include LunaCoreRuntime/src/Modules.cpp
//$Core.Debug

// ----------------------------------------------------------------------------

/*
- Displays a notification on screen
## msg: string
### Core.Debug.message
*/
static int l_Debug_message(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);

    Core::Debug::Message(msg);
    return 0;
}

/*
- Appends the message to log file. Optionally shows the message on screen
## msg: string
## showOnScreen: boolean?
### Core.Debug.log
*/
static int l_Debug_log(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    bool showOnScreen = false;
    if (lua_type(L, 2) == LUA_TBOOLEAN)
        showOnScreen = lua_toboolean(L, 2);
    Core::Debug::LogInfo(msg);
    return 0;
}

/*
- Appends the error message to log file and shows it on screen
## msg: string
### Core.Debug.logerror
*/
static int l_Debug_logerror(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    Core::Debug::LogError(msg);
    return 0;
}

/*
- Show error on screen
## msg: string
### Core.Debug.error
*/

static const luaL_Reg debug_functions[] =
{
    {"message", l_Debug_message},
    {"log", l_Debug_log},
    {"logerror", l_Debug_logerror},
    {"error", l_Debug_logerror},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

bool Core::Module::RegisterDebugModule(lua_State *L)
{
    lua_getglobal(L, "Core");
    luaC_register_field(L, debug_functions, "Debug");
    lua_pop(L, 1);
    return true;
}