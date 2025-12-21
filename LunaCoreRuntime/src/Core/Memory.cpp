#include "Core/Memory.hpp"

#include <string>
#include <cstdlib>

#include <ffi.h>

#include <CTRPluginFramework.hpp>

namespace CTRPF = CTRPluginFramework;

// ----------------------------------------------------------------------------

//$Core.Memory

// ----------------------------------------------------------------------------

/*
- Reads an unsigned integer of 32 bits from memory
## offset: integer
## return: integer?
### Core.Memory.readU32
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

/*
- Reads a signed integer of 32 bits from memory
## offset: integer
## return: integer?
### Core.Memory.readS32
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

/*
- Reads an unsigned integer of 16 bits from memory
## offset: integer
## return: integer?
### Core.Memory.readU16
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

/*
- Reads a signed integer of 16 bits from memory
## offset: integer
## return: integer?
### Core.Memory.readS16
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

/*
- Reads an unsigned integer of 8 bits from memory
## offset: integer
## return: integer?
### Core.Memory.readU8
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

/*
- Reads a signed integer of 8 bits from memory
## offset: integer
## return: integer?
### Core.Memory.readS8
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

/*
- Reads a float from memory
## offset: integer
## return: number?
### Core.Memory.readFloat
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

/*
- Reads a double from memory
## offset: integer
## return: number?
### Core.Memory.readDouble
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

/*
- Reads a string from memory
## offset: integer
## size: integer
## return: string?
### Core.Memory.readString
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

/*
- Writes a signed integer of 32 bits to memory offset. Does the same as the unsigned version, added just to avoid confusion
## offset: integer
## value: integer
## return: boolean
### Core.Memory.writeS32
*/
/*
- Writes an unsigned integer of 32 bits to memory offset
## offset: integer
## value: integer
## return: boolean
### Core.Memory.writeU32
*/
static int l_Memory_writeU32(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    s32 value = (s32)luaL_checknumber(L, 2);
    lua_pushboolean(L, CTRPF::Process::Write32(offset, (u32)value));
    return 1;
}

/*
- Writes a signed integer of 16 bits to memory offset. Does the same as the unsigned version, added just to avoid confusion
## offset: integer
## value: integer
## return: boolean
### Core.Memory.writeS16
*/
/*
- Writes an unsigned integer of 16 bits to memory offset
## offset: integer
## value: integer
## return: boolean
### Core.Memory.writeU16
*/
static int l_Memory_writeU16(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    s16 value = (s16)luaL_checknumber(L, 2);
    lua_pushboolean(L, CTRPF::Process::Write16(offset, (u16)value));
    return 1;
}

/*
- Writes a signed integer of 8 bits to memory offset. Does the same as the unsigned version, added just to avoid confusion
## offset: integer
## value: integer
## return: boolean
### Core.Memory.writeS8
*/
/*
- Writes an unsigned integer of 8 bits to memory offset
## offset: integer
## value: integer
## return: boolean
### Core.Memory.writeU8
*/
static int l_Memory_writeU8(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    s8 value = (s8)luaL_checknumber(L, 2);
    lua_pushboolean(L, CTRPF::Process::Write8(offset, (u8)value));
    return 1;
}

/*
- Writes a float to memory offset
## offset: integer
## value: number
## return: boolean
### Core.Memory.writeFloat
*/
static int l_Memory_writeFloat(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    float value = (float)luaL_checknumber(L, 2);
    lua_pushboolean(L, CTRPF::Process::WriteFloat(offset, value));
    return 1;
}

/*
- Writes a double to memory offset
## offset: integer
## value: number
## return: boolean
### Core.Memory.writeDouble
*/
static int l_Memory_writeDouble(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    double value = (double)luaL_checknumber(L, 2);
    lua_pushboolean(L, CTRPF::Process::WriteDouble(offset, value));
    return 1;
}

/*
- Writes a string to memory offset
## offset: integer
## s: string
## size: integer
## return: boolean
### Core.Memory.writeString
*/
static int l_Memory_writeString(lua_State *L) {
    u32 offset = (u32)luaL_checknumber(L, 1);
    const char* str = luaL_checkstring(L, 2);
    size_t size = (size_t)luaL_checknumber(L, 3);
    lua_pushboolean(L, CTRPF::Process::WriteString(offset, str, size, CTRPF::StringFormat::Utf8));
    return 1;
}

/*
- Allocates memory and returns the start offset
## size: integer
## return: integer?
### Core.Memory.malloc
*/
static int l_Memory_malloc(lua_State *L) {
    size_t size = (size_t)luaL_checknumber(L, 1);
    void* ptr = malloc(size);
    if (ptr) 
        lua_pushnumber(L, (u32)ptr);
    else 
        lua_pushnil(L);
    return 1;
}

/*
- Free memory allocated with malloc
## offset: integer
### Core.Memory.free
*/
static int l_Memory_free(lua_State *L) {
    void* offset = (void*)(u32)luaL_checknumber(L, 1);
    if (offset) 
        free(offset);
    return 0;
}

/*
- Allows to call a function from memory
## foffset: integer
## argstype: string
## returntype: string
## ...
## return: number
### Core.Memory.call
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
    {"writeU32", l_Memory_writeU32},
    {"writeS32", l_Memory_writeU32},  // Works the same for signed
    {"writeU16", l_Memory_writeU16},
    {"writeS16", l_Memory_writeU16},  // Works the same for signed
    {"writeU8", l_Memory_writeU8},
    {"writeS8", l_Memory_writeU8}, // Works the same for signed
    {"writeFloat", l_Memory_writeFloat},
    {"writeDouble", l_Memory_writeDouble},
    {"writeString", l_Memory_writeString},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

bool Core::Module::RegisterMemoryModule(lua_State *L) {
    lua_getglobal(L, "Core");
    luaC_register_field(L, memory_functions, "Memory");
    lua_pop(L, 1);
    return true;
}