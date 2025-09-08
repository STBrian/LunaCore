#include "SharedFunctions.hpp"

#include <cstdlib>
#include <cstring>

#include "Core/Debug.hpp"

Core::SharedFunctions::SharedFunctions() {
    this->cstdlib = new struct cstdlib_s;
    this->cstdlib->malloc = std::malloc;
    this->cstdlib->realloc = std::realloc;
    this->cstdlib->free = std::free;

    this->cstring = new struct cstring_s;
    this->cstring->strlen = std::strlen;
    this->cstring->strcpy = std::strcpy;
    this->cstring->strcmp = std::strcmp;
    this->cstring->strcat = std::strcat;

    this->lua = new struct lua_s;
    this->lua->pcall = lua_pcall;
    this->lua->error = lua_error;
    this->lua->settop = lua_settop;
    this->lua->gettop = lua_gettop;
    this->lua->setfield = lua_setfield;
    this->lua->getfield = lua_getfield;

    this->lua->remove = lua_remove;

    this->lua->createtable = lua_createtable;
    this->lua->newuserdata = lua_newuserdata;

    this->lua->pushvalue = lua_pushvalue;
    this->lua->pushnil = lua_pushnil;
    this->lua->pushboolean = lua_pushboolean;
    this->lua->pushinteger = lua_pushinteger;
    this->lua->pushnumber = lua_pushnumber;
    this->lua->pushstring = lua_pushstring;
    this->lua->pushlstring = lua_pushlstring;
    this->lua->pushfstring = lua_pushfstring;
    this->lua->pushcclosure = lua_pushcclosure;

    this->lua->tolstring = lua_tolstring;

    this->luaL = new luaL_s;
    this->luaL->checknumber = luaL_checknumber;
    this->luaL->checkinteger = luaL_checkinteger;
    this->luaL->checklstring = luaL_checklstring;

    this->luaL->lregister = luaL_register;

    this->debug = new debug_s;
    this->debug->LogMessage = Core::Debug::LogMessage;
    this->debug->LogError = Core::Debug::LogError;
    this->debug->Message = Core::Debug::Message;
    this->debug->Error = Core::Debug::Error;

    this->fslib = new struct fslib_s;
    this->fslib->fopen = Filesystem::fopen;
    this->fslib->fclose = Filesystem::fclose;
    this->fslib->fseek = Filesystem::fseek;
    this->fslib->ftell = Filesystem::ftell;
    this->fslib->rewind = Filesystem::rewind;
    this->fslib->fwrite = Filesystem::fwrite;
    this->fslib->fread = Filesystem::fread;
}