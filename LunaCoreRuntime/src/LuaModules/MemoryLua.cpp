#include "LuaModules.hpp"

#include <string>
#include <cstdlib>

#include <ffi.h>
#include <CTRPluginFramework.hpp>

#include "Helpers/LuaTable.hpp"
#include "game/gstdlib.h"

namespace CTRPF = CTRPluginFramework;

// ----------------------------------------------------------------------------

/* @doc:v2
@module Core.Memory
@desc Process memory read and write operations
*/

// ----------------------------------------------------------------------------

/* @doc:v2
@func Core.Memory.readU32(offset: integer): integer?
@desc Reads an unsigned integer of 32 bits from memory
@param offset integer
@return integer?
*/
static int l_Memory_readU32(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    u32 value = 0;
    if (!CTRPF::Process::Read32(offset, value))
        lua_pushnil(L);
    else
        lua_pushnumber(L, value);
    return 1;
}

/* @doc:v2
@func Core.Memory.readS32(offset: integer): integer?
@desc Reads a signed integer of 32 bits from memory
@param offset integer
@return integer?
*/
static int l_Memory_readS32(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    u32 value = 0;
    if (!CTRPF::Process::Read32(offset, value))
        lua_pushnil(L);
    else {
        lua_pushnumber(L, (s32)value);
    }
    return 1;
}

/* @doc:v2
@func Core.Memory.readU16(offset: integer): integer?
@desc Reads an unsigned integer of 16 bits from memory
@param offset integer
@return integer?
*/
static int l_Memory_readU16(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    u16 value = 0;
    if (!CTRPF::Process::Read16(offset, value))
        lua_pushnil(L);
    else
        lua_pushnumber(L, value);
    return 1;
}

/* @doc:v2
@func Core.Memory.readS16(offset: integer): integer?
@desc Reads a signed integer of 16 bits from memory
@param offset integer
@return integer?
*/
static int l_Memory_readS16(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    u16 value = 0;
    if (!CTRPF::Process::Read16(offset, value))
        lua_pushnil(L);
    else {
        lua_pushnumber(L, (s16)value);
    }
    return 1;
}

/* @doc:v2
@func Core.Memory.readU8(offset: integer): integer?
@desc Reads an unsigned integer of 8 bits from memory
@param offset integer
@return integer?
*/
static int l_Memory_readU8(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    u8 value = 0;
    if (!CTRPF::Process::Read8(offset, value))
        lua_pushnil(L);
    else
        lua_pushnumber(L, value);
    return 1;
}

/* @doc:v2
@func Core.Memory.readS8(offset: integer): integer?
@desc Reads a signed integer of 8 bits from memory
@param offset integer
@return integer?
*/
static int l_Memory_readS8(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    u8 value = 0;
    if (!CTRPF::Process::Read8(offset, value))
        lua_pushnil(L);
    else {
        lua_pushnumber(L, (s8)value);
    }
    return 1;
}

/* @doc:v2
@func Core.Memory.readFloat(offset: integer): number?
@desc Reads a float from memory
@param offset integer
@return number?
*/
static int l_Memory_readFloat(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    float value = 0;
    if (!CTRPF::Process::ReadFloat(offset, value))
        lua_pushnil(L);
    else
        lua_pushnumber(L, value);
    return 1;
}

/* @doc:v2
@func Core.Memory.readDouble(offset: integer): number?
@desc Reads a double from memory
@param offset integer
@return number?
*/
static int l_Memory_readDouble(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    double value = 0;
    if (!CTRPF::Process::ReadDouble(offset, value))
        lua_pushnil(L);
    else
        lua_pushnumber(L, value);
    return 1;
}

/* @doc:v2
@func Core.Memory.readString(offset: integer, size: integer): string?
@desc Reads a string from memory
@param offset integer
@param size integer
@return string?
*/
static int l_Memory_readString(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    size_t size = (size_t)luaL_checknumber(L, 2);
    std::string value = "";
    if (!CTRPF::Process::ReadString(offset, value, size, CTRPF::StringFormat::Utf8))
        lua_pushnil(L);
    else
        lua_pushstring(L, value.c_str());
    return 1;
}

/* @doc:v2
@func Core.Memory.writeS32(offset: integer, value: integer): boolean
@desc Writes a signed integer of 32 bits to memory offset. Does the same as the unsigned version, added just to avoid confusion
@param offset integer
@param value integer
@return boolean
*/
/* @doc:v2
@func Core.Memory.writeU32(offset: integer, value: integer): boolean
@desc Writes an unsigned integer of 32 bits to memory offset
@param offset integer
@param value integer
@return boolean
*/
static int l_Memory_writeU32(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    s32 value = (s32)luaL_checknumber(L, 2);
    lua_pushboolean(L, CTRPF::Process::Write32(offset, (u32)value));
    return 1;
}

class WriteFuncHelper {
    public:
    static inline bool Write(u32 address, u32 value) {
        return CTRPF::Process::Write32(address, value);
    }

    static inline bool Write(u32 address, u16 value) {
        return CTRPF::Process::Write16(address, value);
    }

    static inline bool Write(u32 address, u8 value) {
        return CTRPF::Process::Write8(address, value);
    }

    static inline bool Write(u32 address, float value) {
        return CTRPF::Process::WriteFloat(address, value);
    }

    static inline bool Write(u32 address, double value) {
        return CTRPF::Process::WriteDouble(address, value);
    }
};

template <typename T>
requires std::is_arithmetic_v<T>
static int l_Memory_write(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    if constexpr (std::is_integral_v<T>) {
        using ST = std::make_signed_t<T>;
        using UT = std::make_unsigned_t<T>;

        ST value = (ST)luaL_checknumber(L, 2);
        lua_pushboolean(L, WriteFuncHelper::Write(offset, (UT)value));
    } else if constexpr (std::is_floating_point_v<T>) {
        T value = (T)luaL_checknumber(L, 2);
        lua_pushboolean(L, WriteFuncHelper::Write(offset, value));
    } else {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "unsupported type");
    }
    return 1;
}

/* @doc:v2
@func Core.Memory.writeS16(offset: integer, value: integer): boolean
@desc Writes a signed integer of 16 bits to memory offset. Does the same as the unsigned version, added just to avoid confusion
@param offset integer
@param value integer
@return boolean
*/
/* @doc:v2
@func Core.Memory.writeU16(offset: integer, value: integer): boolean
@desc Writes an unsigned integer of 16 bits to memory offset
@param offset integer
@param value integer
@return boolean
*/

/* @doc:v2
@func Core.Memory.writeS8(offset: integer, value: integer): boolean
@desc Writes a signed integer of 8 bits to memory offset. Does the same as the unsigned version, added just to avoid confusion
@param offset integer
@param value integer
@return boolean
*/
/* @doc:v2
@func Core.Memory.writeU8(offset: integer, value: integer): boolean
@desc Writes an unsigned integer of 8 bits to memory offset
@param offset integer
@param value integer
@return boolean
*/

/* @doc:v2
@func Core.Memory.writeFloat(offset: integer, value: number): boolean
@desc Writes a float to memory offset
@param offset integer
@param value number
@return boolean
*/

/* @doc:v2
@func Core.Memory.writeDouble(offset: integer, value: number): boolean
@desc Writes a double to memory offset
@param offset integer
@param value number
@return boolean
*/

/* @doc:v2
@func Core.Memory.writeString(offset: integer, s: string, size: integer): boolean
@desc Writes a string to memory offset
@param offset integer
@param s string
@param size integer
@return boolean
*/
static int l_Memory_writeString(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    const char* str = luaL_checkstring(L, 2);
    size_t size = (size_t)luaL_checknumber(L, 3);
    lua_pushboolean(L, CTRPF::Process::WriteString(offset, str, size, CTRPF::StringFormat::Utf8));
    return 1;
}

/* @doc:v2
@func Core.Memory.malloc(size: integer, useGameHeap: boolean?): integer?
@desc Allocates memory and returns the start offset
@param size integer
@param useGameHeap boolean?
@return integer?
*/
static int l_Memory_malloc(lua_State *L) {
    size_t size = (size_t)luaL_checknumber(L, 1);
    bool useGameHeap = false;
    if (lua_gettop(L) >= 2 && lua_isboolean(L, 2)) {
        useGameHeap = lua_toboolean(L, 2);
    }
    void* ptr;
    if (useGameHeap)
        ptr = gstd_malloc(size);
    else
        ptr = malloc(size);
    if (ptr) 
        lua_pushnumber(L, (u32)ptr);
    else 
        lua_pushnil(L);
    return 1;
}

/* @doc:v2
@func Core.Memory.free(offset: integer, useGameHeap: boolean?)
@desc Free memory allocated with malloc
@param offset integer
@param useGameHeap boolean?
*/
static int l_Memory_free(lua_State *L) {
    void* offset = (void*)(u32)luaL_checknumber(L, 1);
    bool useGameHeap = false;
    if (lua_gettop(L) >= 2 && lua_isboolean(L, 2)) {
        useGameHeap = lua_toboolean(L, 2);
    }
    if (offset) {
        if (useGameHeap)
            gstd_free(offset);
        else
            free(offset);
    }
    return 0;
}

/* @doc:v2
@func Core.Memory.call(foffset: integer, argstype: string, returntype: string, ...: any): number
@desc Allows to call a function from memory
@param foffset integer
@param argstype string
@param returntype string
@param ... any
@return number
*/
static int l_Memory_call(lua_State *L) {
    u32 foffset = (u32)luaL_checknumber(L, 1);
    size_t nargs, lenreturnType;
    const char* typeStr = luaL_checklstring(L, 2, &nargs);
    const char* returnType = luaL_checklstring(L, 3, &lenreturnType);
    ffi_cif cif;
    ffi_type* args[nargs];
    void* values[nargs];
    float floatvalues[nargs];
    s32 intvalues[nargs];
    u32 intresult = 0;
    float floatresult = 0;
    bool returnsFloat = false;
    bool signedValue = false;
    bool returnsValue = lenreturnType != 0;
    if (returnsValue) {
        switch (returnType[0]) {
            case 'c': case 'b': case 'h': case 'i': case 'l':
                signedValue = true;
                break;
            case 'B': case 'H': case 'I': case 'L': case '?': case 'P':
                signedValue = false;
                break;
            case 'f':
                returnsFloat = true;
                break;
            default:
                return luaL_error(L, "bad return type: %c", returnType[0]);
                break;
        }
    }
    for (int i = 0; i < nargs; i++) {
        switch (typeStr[i]) {
            case 'c': case 'b':
                args[i] = &ffi_type_sint8;
                if (!lua_isnumber(L, 4 + i))
                    return luaL_error(L, "type %d doesn't match char: %s", i, luaL_typename(L, 4 + i));
                break;
            case 'B':
                args[i] = &ffi_type_uint8;
                if (!lua_isnumber(L, 4 + i))
                    return luaL_error(L, "type %d doesn't match unsigned char: %s", i, luaL_typename(L, 4 + i));
                break;
            case '?':
                args[i] = &ffi_type_uint8;
                if (!lua_isboolean(L, 4 + i))
                    return luaL_error(L, "type %d doesn't match boolean: %s", i, luaL_typename(L, 4 + i));
                break;
            case 'h':
                args[i] = &ffi_type_sint16;
                if (!lua_isnumber(L, 4 + i))
                    return luaL_error(L, "type %d doesn't match short: %s", i, luaL_typename(L, 4 + i));
                break;
            case 'H':
                args[i] = &ffi_type_uint16;
                if (!lua_isnumber(L, 4 + i))
                    return luaL_error(L, "type %d doesn't match unsigned short: %s", i, luaL_typename(L, 4 + i));
                break;
            case 'i': case 'l':
                args[i] = &ffi_type_sint32;
                if (!lua_isnumber(L, 4 + i))
                    return luaL_error(L, "type %d doesn't match int: %s", i, luaL_typename(L, 4 + i));
                break;
            case 'I': case 'L':
                args[i] = &ffi_type_uint32;
                if (!lua_isnumber(L, 4 + i))
                    return luaL_error(L, "type %d doesn't match unsigned int: %s", i, luaL_typename(L, 4 + i));
                break;
            case 'f':
                args[i] = &ffi_type_float;
                if (!lua_isnumber(L, 4 + i))
                    return luaL_error(L, "type %d doesn't match float: %s", i, luaL_typename(L, 4 + i));
                break;
            case 's': 
                args[i] = &ffi_type_pointer;
                if (!lua_isstring(L, 4 + i))
                    return luaL_error(L, "type %d doesn't match string: %s", i, luaL_typename(L, 4 + i));
                break;
            case 'P':
                args[i] = &ffi_type_pointer;
                if (!lua_isnumber(L, 4 + i) && !lua_isuserdata(L, 4 + i))
                    return luaL_error(L, "type %d doesn't match pointer: %s", i, luaL_typename(L, 4 + i));
                break;
            default:
                return luaL_error(L, "bad type: %c", typeStr[i]);
                break;
        }
    }
    for (int i = 0; i < nargs; i++) {
        switch (typeStr[i]) {
            case 'c': case 'b': case 'B': case 'h': case 'H': case 'i': case 'I': case 'l': case 'L':
                intvalues[i] = (s32)lua_tonumber(L, 4 + i);
                values[i] = &intvalues[i];
                break;
            case '?':
                intvalues[i] = (bool)lua_toboolean(L, 4 + i);
                values[i] = &intvalues[i];
                break;
            case 'f':
                floatvalues[i] = (float)lua_tonumber(L, 4 + i);
                values[i] = &floatvalues[i];
                break;
            case 's':
                intvalues[i] = (s32)(void*)lua_tostring(L, 4 + i);
                values[i] = &intvalues[i];
                break;
            case 'P': {
                if (lua_isnumber(L, 4 + i))
                    intvalues[i] = (s32)lua_tonumber(L, 4 + i);
                else
                    intvalues[i] = (u32)lua_touserdata(L, 4 + i);
                values[i] = &intvalues[i];
                break;
            }
        }
    }
    if (returnsValue) {
        if (returnsFloat) {
            ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, &ffi_type_float, args);
            ffi_call(&cif, FFI_FN((void*)foffset), &floatresult, values);
            lua_pushnumber(L, floatresult);
            return 1;
        } else {
            ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, &ffi_type_sint32, args);
            ffi_call(&cif, FFI_FN((void*)foffset), &intresult, values);
            if (signedValue)
                lua_pushnumber(L, (s32)intresult);
            else
                lua_pushnumber(L, (u32)intresult);
            return 1;
        }
    }
    else {
        ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, &ffi_type_void, args);
        ffi_call(&cif, FFI_FN((void*)foffset), nullptr, values);
        return 0;
    }
}

// I'm going to use templates to avoid bloating the code of repetitive functions
template <typename searchType>
static int l_Memory_search(lua_State* L) {
    u32 start = luaL_checknumber(L, 1);
    u32 size = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    std::vector<searchType> pattern;
    LuaTable pattern_table(L, 3);
    for (auto val : pattern_table) {
        pattern.push_back((searchType)val.second);
    }
    u32 result = CTRPF::Utils::Search(start, size, pattern);
    lua_pushnumber(L, result);
    return 1;
}

// ----------------------------------------------------------------------------

static const luaL_Reg memory_functions[] = {
    {"malloc", l_Memory_malloc},
    {"free", l_Memory_free},
    {"call", l_Memory_call},
    {"readU32", l_Memory_readU32},
    {"readS32", l_Memory_readS32},
    {"readU16", l_Memory_readU16},
    {"readS16", l_Memory_readS16},
    {"readU8", l_Memory_readU8},
    {"readS8", l_Memory_readS8},
    {"readFloat", l_Memory_readFloat},
    {"readDouble", l_Memory_readDouble},
    {"readString", l_Memory_readString},
    {"writeU32", l_Memory_write<u32>},
    {"writeS32", l_Memory_write<u32>},  // Works the same for signed
    {"writeU16", l_Memory_write<u16>},
    {"writeS16", l_Memory_write<u16>},  // Works the same for signed
    {"writeU8", l_Memory_write<u8>},
    {"writeS8", l_Memory_write<u8>}, // Works the same for signed
    {"writeFloat", l_Memory_write<float>},
    {"writeDouble", l_Memory_write<double>},
    {"writeString", l_Memory_writeString},
    {"searchU32", l_Memory_search<u32>},
    {"searchU16", l_Memory_search<u16>},
    {"searchU8", l_Memory_search<u8>},
    {"searchFloat", l_Memory_search<float>},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

bool Core::Module::RegisterMemoryModule(lua_State *L) {
    lua_getglobal(L, "Core");
    luaC_register_field(L, memory_functions, "Memory");
    lua_pop(L, 1);
    return true;
}