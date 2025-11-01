#include "Core/Utils/Utils.hpp"

#include <cstdlib>

#include <CTRPluginFramework.hpp>

#include "Core/Debug.hpp"

#define USA_REG 0x00040000001B8700LL
#define EUR_REG 0x000400000017CA00LL
#define JPN_REG 0x000400000017FD00LL
#define IS_VUSA_COMP(id, version) ((id) == USA_REG && (version) == 9408) // 1.9.19 USA
#define IS_VEUR_COMP(id, version) ((id) == EUR_REG && (version) == 9392) // 1.9.19 EUR
#define IS_VJPN_COMP(id, version) ((id) == JPN_REG && (version) == 9424) // 1.9.19 JPN
#define IS_TARGET_ID(id) ((id) == USA_REG || (id) == EUR_REG || (id) == JPN_REG)

namespace CTRPF = CTRPluginFramework;

namespace Core::Utils {
    bool checkTitle() {
        u64 titleId = CTRPF::Process::GetTitleID();
        return IS_TARGET_ID(titleId);
    }

    void getRegion(std::string& reg) {
        u64 titleId = CTRPF::Process::GetTitleID();
        if (titleId == USA_REG)
            reg.assign("USA");
        else if (titleId == EUR_REG)
            reg.assign("EUR");
        else if (titleId == JPN_REG)
            reg.assign("JPN");
    }

    bool checkCompatibility() {
        u64 titleId = CTRPF::Process::GetTitleID();
        u16 gameVer = CTRPF::Process::GetVersion();
        return (IS_VUSA_COMP(titleId, gameVer) || IS_VEUR_COMP(titleId, gameVer) || IS_VJPN_COMP(titleId, gameVer));
    }

    std::string formatTime(time_t time) {
        struct tm *timeInfo = localtime(&time);
        std::string outStr;
        if (timeInfo->tm_hour < 10)
            outStr += "0";
        outStr += std::to_string(timeInfo->tm_hour)+":";
        if (timeInfo->tm_min < 10)
            outStr += "0";
        outStr += std::to_string(timeInfo->tm_min)+":";
        if (timeInfo->tm_sec < 10)
            outStr += "0";
        outStr += std::to_string(timeInfo->tm_sec);
        return outStr;
    }

    std::string strip(const std::string &str) {
        if (str.empty())
            return "";
        size_t startPos = 0;
        size_t endPos = str.size();
        while (str[startPos] == ' ' && startPos < str.size())
            startPos++;
        while (str[endPos - 1] == ' ' && endPos > startPos)
            endPos--;
        if (startPos == endPos)
            return "";
        std::string newStr = str.substr(startPos, endPos - startPos);
        return newStr;
    }

    std::string LoadFile(const std::string& filepath) {
        std::string content;
        if (!CTRPF::File::Exists(filepath))
            return content;
        CTRPF::File file_ptr;
        CTRPF::File::Open(file_ptr, filepath, CTRPF::File::READ);
        if (!file_ptr.IsOpen())
            return content;
        file_ptr.Seek(0, CTRPF::File::SeekPos::END);
        size_t fileSize = file_ptr.Tell();
        file_ptr.Seek(0, CTRPF::File::SeekPos::SET);
        char *fileContent = new char[fileSize + 1];
        if (fileContent == NULL) 
            return content;
        file_ptr.Read(fileContent, fileSize);
        fileContent[fileSize] = '\0';
        content = std::string(fileContent);
        free(fileContent);
        return content;
    }
}

static bool ReplaceStringWithPointer(u32 insAddr, u32 strAddr, u32 ptrAddr, u8 reg) {
    u32 baseIns = (0xe5bf << 16)|((reg & 0xF) << 12);
    u32 addrsOffset = strAddr - insAddr - 8;
    if (addrsOffset & 0xFFF != addrsOffset)
        return false;
    if (!CTRPF::Process::Write32(insAddr, baseIns|addrsOffset)) return false; // Patch instruction
    return CTRPF::Process::Write32(strAddr, ptrAddr); // Write pointer to offset
}

static bool ReplaceConstString(u32 insAddr, u32 strAddr, u8 reg, const std::string& text) {
    char *textPtr = (char*)GameCalloc(text.size() + 1); // Allocate string space in game memory
    if (textPtr == NULL) return false;
    if (!CTRPF::Process::WriteString((u32)textPtr, text.c_str(), text.size() + 1)) return false; // Copy string
    return ReplaceStringWithPointer(insAddr, strAddr, (u32)textPtr, reg);
}

bool Core::RegisterUtilsModule(lua_State *L)
{
    const char *lua_Code = R"(
        function readOnlyTable(tbl, tblName)
            local mt_newindex_func = function (_, key, _)
                error(tblName.." is read-only")
            end
            return setmetatable({}, {__index = tbl, __newindex = mt_newindex_func, __metatable = "protected"})
        end
    )";
    if (luaL_dostring(L, lua_Code))
    {
        Core::Debug::LogError("Core::Utils::Load error: " + std::string(lua_tostring(L, -1)));
        lua_pop(L, 1);
        return false;
    }
    return true;
}

void Core::UnregisterUtilsModule(lua_State *L)
{
    lua_pushnil(L);
    lua_setglobal(L, "readOnlyTable");
    lua_gc(L, LUA_GCCOLLECT, 0);
}