#pragma once

#ifndef __LCEX__
#include "lua_common.h"
#else
#include <stddef.h>
#define LUA_REGISTRYINDEX	(-10000)
#define LUA_ENVIRONINDEX	(-10001)
#define LUA_GLOBALSINDEX	(-10002)
#define lua_upvalueindex(i)	(LUA_GLOBALSINDEX-(i))
typedef struct lua_State lua_State;
typedef int (*lua_CFunction) (lua_State *L);
typedef ptrdiff_t lua_Integer;
typedef double lua_Number;
typedef struct luaL_Reg {
  const char *name;
  lua_CFunction func;
} luaL_Reg;
#endif

// To ensure that extensions for old versions work in new versions without
// needing to recompile every time an extension for each version
// this SharedFunctions class was made to expose runtime functions to extensions

namespace Core {
    class SharedFunctions {
        public:
        // Only add new modules at the end of existing ones
        // Only add new functions at the end of existing ones inside a struct
        struct cstdlib_s {
            void *(*malloc)(size_t);
            void *(*realloc)(void*, size_t);
            void (*free)(void*);
        };
        struct cstdlib_s* cstdlib;

        struct cstring_s {
            size_t (*strlen)(const char*);
            char *(*strcpy)(char*, const char*);
            int (*strcmp)(const char*, const char*);
            char *(*strcat)(char*, const char*);
        };
        struct cstring_s* cstring;

        struct lua_s {
            int (*pcall)(lua_State*, int, int, int);
            int (*error)(lua_State*);
            void (*settop)(lua_State*, int);
            int (*gettop)(lua_State*);
            void (*setfield)(lua_State*, int, const char*);
            void (*getfield)(lua_State*, int, const char*);

            void (*createtable)(lua_State*, int, int);
            void *(*newuserdata)(lua_State*, size_t);

            void (*pushvalue)(lua_State*, int);
            void (*pushnil)(lua_State*);
            void (*pushboolean)(lua_State*, int);
            void (*pushinteger)(lua_State*, lua_Integer);
            void (*pushnumber)(lua_State*, lua_Number);
            void (*pushstring)(lua_State*, const char*);
            void (*pushlstring)(lua_State*, const char*, size_t);
            const char *(*pushfstring)(lua_State*, const char*, ...);
            void (*pushcclosure)(lua_State*, lua_CFunction, int);
        };
        struct lua_s* lua;

        struct luaL_s {
            lua_Number (*checknumber)(lua_State*, int);
            lua_Integer (*checkinteger)(lua_State*, int);
            const char *(*checklstring)(lua_State*, int, size_t*);
            void (*lregister)(lua_State*, const char*, const luaL_Reg*);
        };
        struct luaL_s* luaL;

        struct debug_s {
            void (*LogMessage)(const char*, bool);
            void (*LogError)(const char*);
            void (*Message)(const char*);
            void (*Error)(const char*);
        };
        struct debug_s* debug;

        SharedFunctions();
    };
}