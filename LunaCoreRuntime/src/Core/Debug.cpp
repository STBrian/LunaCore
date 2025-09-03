#include "Core/Debug.hpp"

#include <string>
#include <time.h>
#include <CTRPluginFramework.hpp>

#include "Core/System.hpp"
#include "Core/Utils/Utils.hpp"

namespace CTRPF = CTRPluginFramework;

static CTRPF::File logFile;

const char* Core::Debug::tab = "    ";

bool Core::Debug::OpenLogFile(const STRING_CLASS& filepath)
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

void Core::Debug::LogRaw(const STRING_CLASS& msg)
{
    std::string newMsg(msg);
    Core::Utils::Replace(newMsg, "\t", "    ");
    logFile.Write(newMsg.c_str(), newMsg.size());
    logFile.Flush();
}

static void DebugWriteLog(const STRING_CLASS& msg)
{
    if (logFile.IsOpen()) {
        std::string out_msg = "[";
        out_msg += Core::Utils::formatTime(Core::System::getTime()) + "] " + std::string(msg);
        Core::Debug::LogRaw(out_msg + "\n");
    }
}

void Core::Debug::LogMessage(const STRING_CLASS& msg, bool showOnScreen) {
    if (showOnScreen)
        CTRPF::OSD::Notify(msg);
    DebugWriteLog(msg);
}

void Core::Debug::LogMessage(const char* msg, bool showOnScreen) {
    LogMessage(std::string(msg), showOnScreen);
}

void Core::Debug::LogError(const STRING_CLASS& msg) {
    Core::Debug::Error(msg);
    DebugWriteLog("[ERROR] " + msg);
}

void Core::Debug::LogError(const char* msg) {
    LogError(std::string(msg));
}

void Core::Debug::Message(const STRING_CLASS& msg) {
    CTRPF::OSD::Notify(msg);
}

void Core::Debug::Message(const char* msg) {
    CTRPF::OSD::Notify(msg);
}

void Core::Debug::Error(const STRING_CLASS& msg) {
    CTRPF::OSD::Notify(msg, CTRPF::Color::Red, CTRPF::Color::Black);
}

void Core::Debug::Error(const char* msg) {
    CTRPF::OSD::Notify(msg, CTRPF::Color::Red, CTRPF::Color::Black);
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
## showOnScreen: boolean
### Core.Debug.log
*/
static int l_Debug_log(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    bool showOnScreen = false;
    if (lua_type(L, 2) == LUA_TBOOLEAN)
        showOnScreen = lua_toboolean(L, 2);
    Core::Debug::LogMessage(msg, showOnScreen);
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
static int l_Debug_error(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    Core::Debug::Error(msg);
    return 0;
}

static const luaL_Reg debug_functions[] =
{
    {"message", l_Debug_message},
    {"log", l_Debug_log},
    {"logerror", l_Debug_logerror},
    {"error", l_Debug_error},
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