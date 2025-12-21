#include "Core/GameModules/Player/Camera.hpp"

#include <CTRPluginFramework.hpp>

#include "game/Minecraft.hpp"
#include "game/memory.hpp"
#include "string_hash.hpp"

#include "lua_utils.hpp"

namespace CTRPF = CTRPluginFramework;

enum player_camera_offsets : u32 {
    itemCameraFOV = 0x3CEE80,
};

static float* getCameraFOVPtr() {
    gstd::alloc_ptr<u32**> gPtr(*reinterpret_cast<u32****>(0xa32694)); // Hard work to get this path
    if (gPtr) {
        gstd::alloc_ptr<u32*> ptr1(*gPtr.get());
        if (ptr1) {
            gstd::alloc_ptr<u32> ptr2(*(ptr1 + 2));
            if (ptr2) return reinterpret_cast<float*>((u32)ptr2.get() + 0x118);
        }
    }
    return NULL;
}

// ----------------------------------------------------------------------------

//!include LunaCoreRuntime/src/Core/Game/Player/Player.cpp
//$Game.LocalPlayer.Camera

// ----------------------------------------------------------------------------

/*
=Game.LocalPlayer.Camera.FOV = 0.0
=Game.LocalPlayer.Camera.Yaw = 0.0
=Game.LocalPlayer.Camera.Pitch = 0.0
*/

static int l_Camera_index(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;

    uint32_t key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    switch (key) {
        case hash("FOV"): {
            float* fov = getCameraFOVPtr();
            lua_pushnumber(L, fov ? *fov : 0.0f);
            break;
        }
        case hash("Yaw"):
            lua_pushnumber(L, Minecraft::GetYaw());
            break;
        case hash("Pitch"):
            lua_pushnumber(L, Minecraft::GetPitch());
            break;
        default:
            valid_key = false;
            break;
    }

    if (valid_key)
        return 1;
    else
        return 0;
}

static int l_Camera_newindex(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return luaL_error(L, "attempt to set field '?' of \"Camera\"");

    // "Safe" c++ error handler
    LUAUTILS_INIT_TYPEERROR_HANDLER();
    LUAUTILS_SET_TYPEERROR_MESSAGE("unable to assign to a \"%s\" field a \"%s\" value");

    {
    uint32_t key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    switch (key) {
        case hash("FOV"): {
            LUAUTILS_CHECKTYPE(L, LUA_TNUMBER, 3);
            float* fov = getCameraFOVPtr();
            if (fov)
                *fov = lua_tonumber(L, 3);
            *reinterpret_cast<float*>(player_camera_offsets::itemCameraFOV) = lua_tonumber(L, 3);
            break;
        }
        case hash("Yaw"):
            LUAUTILS_CHECKTYPE(L, LUA_TNUMBER, 3);
            Minecraft::SetYaw(lua_tonumber(L, 3));
            break;
        case hash("Pitch"):
            LUAUTILS_CHECKTYPE(L, LUA_TNUMBER, 3);
            Minecraft::SetPitch(lua_tonumber(L, 3));
            break;
        default:
            valid_key = false;
            break;
    }

    if (valid_key)
        return 0;
    else
        return luaL_error(L, "'%s' is not a valid member of object or is read-only value", lua_tostring(L, 2));
    }

    LUAUTILS_SET_TYPEERROR_HANDLER(L);
}

// ----------------------------------------------------------------------------

static inline void RegisterCameraMetatables(lua_State *L)
{
    luaL_newmetatable(L, "CameraMetatable");
    lua_pushcfunction(L, l_Camera_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_Camera_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pop(L, 1);
}

// Required to be called inside LocalPlayer definition
bool Core::Module::LocalPlayer::RegisterCameraModule(lua_State *L)
{
    RegisterCameraMetatables(L);

    lua_newtable(L); // LocalPlayer.Camera
    luaC_setmetatable(L, "CameraMetatable");
    lua_setfield(L, -2, "Camera");
    return true;
}