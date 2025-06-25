#include "Core/Game/Player/Camera.hpp"

#include <CTRPluginFramework.hpp>

#include "Game/Minecraft.hpp"
#include "string_hash.hpp"

namespace CTRPF = CTRPluginFramework;

enum player_camera_offsets : u32 {
    itemCameraFOV = 0x100000 + 0x2CEE80,
    playerCameraFOV1 = 0x341F30F8, // Can change address
    playerCameraFOV2 = 0x341F313C,
};

// ----------------------------------------------------------------------------

//!include Sources/Game/Player/Player.cpp
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
            float fov;
            CTRPF::Process::ReadFloat(player_camera_offsets::playerCameraFOV2, fov);
            lua_pushnumber(L, fov);
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
        return luaL_error(L, "Attempt to set unknown member of object");

    uint32_t key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    switch (key) {
        case hash("FOV"):
            CTRPF::Process::WriteFloat(player_camera_offsets::playerCameraFOV2, luaL_checknumber(L, 3));
            CTRPF::Process::WriteFloat(player_camera_offsets::itemCameraFOV, lua_tonumber(L, 3));
            break;
        case hash("Yaw"):
            Minecraft::SetYaw(luaL_checknumber(L, 3));
            break;
        case hash("Pitch"):
            Minecraft::SetPitch(luaL_checknumber(L, 3));
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
bool Core::Game::LocalPlayer::RegisterCameraModule(lua_State *L)
{
    RegisterCameraMetatables(L);

    lua_newtable(L); // LocalPlayer.Camera
    luaC_setmetatable(L, "CameraMetatable");
    lua_setfield(L, -2, "Camera");
    return true;
}