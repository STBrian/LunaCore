#include "Core/Game/Player/Player.hpp"

#include <CTRPluginFramework.hpp>
#include "Game/Minecraft.hpp"
#include "string_hash.hpp"

#include "Core/Game/Player/Camera.hpp"
#include "Core/Game/Player/Inventory.hpp"

namespace CTRPF = CTRPluginFramework;

enum player_offsets : u32 {
    playerReachDistance = 0x100000 + 0x54FB68,
    playerSwimSpeed = 0x100000 + 0x3EA090,
};

// ----------------------------------------------------------------------------

//$Game.LocalPlayer

// ----------------------------------------------------------------------------

//$Game.LocalPlayer.Position

/*
- Gets local player position
## return: number
## return: number
## return: number
### Game.LocalPlayer.Position.get
*/
static int l_Player_Position_get(lua_State *L)
{
    float x, y, z;
    Minecraft::GetPlayerPosition(x, y, z);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, z);
    return 3;
}

/*
- Sets player position
## x: number
## y: number
## z: number
### Game.LocalPlayer.Position.set
*/
static int l_Player_Position_set(lua_State *L)
{
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float z = luaL_checknumber(L, 3);
    Minecraft::SetPlayerPosition(x, y, z);
    return 0;
}

/*
- Adds X, Y, Z to current player position
## x: number
## y: number
## z: number
### Game.LocalPlayer.Position.add
*/
static int l_Player_Position_add(lua_State *L)
{
    float x2 = luaL_checknumber(L, 1);
    float y2 = luaL_checknumber(L, 2);
    float z2 = luaL_checknumber(L, 3);
    float x1, y1, z1;
    Minecraft::GetPlayerPosition(x1, y1, z1);
    Minecraft::SetPlayerPosition(x1 + x2, y1 + y2, z1 + z2);
    return 0;
}

static const luaL_Reg player_position_methods[] =
{
    {"get", l_Player_Position_get},
    {"set", l_Player_Position_set},
    {"add", l_Player_Position_add},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

//$Game.LocalPlayer.Velocity

/*
- Gets local player velocity
## return: number
## return: number
## return: number
### Game.LocalPlayer.Velocity.get
*/
static int l_Player_Velocity_get(lua_State *L)
{
    float x = Minecraft::GetVelocityX();
    float y = Minecraft::GetVelocityY();
    float z = Minecraft::GetVelocityZ();
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, z);
    return 3;
}

/*
- Sets player velocity
## x: number
## y: number
## z: number
### Game.LocalPlayer.Velocity.set
*/
static int l_Player_Velocity_set(lua_State *L)
{
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float z = luaL_checknumber(L, 3);
    Minecraft::SetVelocityX(x);
    Minecraft::SetVelocityY(y);
    Minecraft::SetVelocityZ(z);
    return 0;
}

/*
- Adds X, Y, Z velocity to current player velocity
## x: number
## y: number
## z: number
### Game.LocalPlayer.Velocity.add
*/
static int l_Player_Velocity_add(lua_State *L)
{
    float x2 = luaL_checknumber(L, 1);
    float y2 = luaL_checknumber(L, 2);
    float z2 = luaL_checknumber(L, 3);
    float x1 = Minecraft::GetVelocityX();
    float y1 = Minecraft::GetVelocityY();
    float z1 = Minecraft::GetVelocityZ();
    Minecraft::SetVelocityX(x1 + x2);
    Minecraft::SetVelocityY(y1 + y2);
    Minecraft::SetVelocityZ(z1 + z2);
    return 0;
}

static const luaL_Reg player_velocity_methods[] =
{
    {"get", l_Player_Velocity_get},
    {"set", l_Player_Velocity_set},
    {"add", l_Player_Velocity_add},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

/*
=Game.LocalPlayer.OnGround = false
=Game.LocalPlayer.Sneaking = false
=Game.LocalPlayer.Jumping = false
=Game.LocalPlayer.Sprinting = false
=Game.LocalPlayer.Flying = false
=Game.LocalPlayer.UnderWater = false
=Game.LocalPlayer.TouchingWall = false
=Game.LocalPlayer.Invincible = false
=Game.LocalPlayer.CanFly = false
=Game.LocalPlayer.CanConsumeItems = false
=Game.LocalPlayer.BaseMoveSpeed = 0.0
=Game.LocalPlayer.MoveSpeed = 0.0
=Game.LocalPlayer.SwimSpeed = 0.02
=Game.LocalPlayer.FlySpeed = 0.0
=Game.LocalPlayer.CurrentHP = 0.0
=Game.LocalPlayer.MaxHP = 0.0
=Game.LocalPlayer.CurrentHunger = 0.0
=Game.LocalPlayer.MaxHunger = 0.0
=Game.LocalPlayer.CurrentLevel = 0.0
=Game.LocalPlayer.LevelProgress = 0.0
=Game.LocalPlayer.Gamemode = 0
=Game.LocalPlayer.ReachDistance = 0.0
=Game.LocalPlayer.SprintDelay = 0.0
=Game.LocalPlayer.Dimension = 0
*/
static int l_LocalPlayer_index(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;

    uint32_t key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    Core::Player* ply = *Core::Player::PlayerInstance;

    switch (key) {
        case hash("OnGround"):
            lua_pushboolean(L, Minecraft::IsPlayerOnGround());
            break;
        case hash("Sneaking"):
            lua_pushboolean(L, Minecraft::IsPlayerSneaking());
            break;
        case hash("Jumping"):
            lua_pushboolean(L, Minecraft::IsPlayerJumping());
            break;
        case hash("Sprinting"):
            lua_pushboolean(L, Minecraft::IsPlayerSprinting());
            break;
        case hash("Flying"):
            lua_pushboolean(L, Minecraft::IsPlayerFlying());
            break;
        case hash("UnderWater"):
            lua_pushboolean(L, Minecraft::IsPlayerUnderWater());
            break;
        case hash("TouchingWall"):
            lua_pushboolean(L, Minecraft::IsPlayerTouchingWall());
            break;
        case hash("Invincible"):
            lua_pushboolean(L, Minecraft::IsInvincible());
            break;
        case hash("CanFly"):
            lua_pushboolean(L, Minecraft::IsFlightAvailable());
            break;
        case hash("CanConsumeItems"):
            lua_pushboolean(L, !Minecraft::IsItemConsumptionDisabled());
            break;
        case hash("BaseMoveSpeed"):
            lua_pushnumber(L, Minecraft::GetBaseMoveSpeed());
            break;
        case hash("MoveSpeed"):
            lua_pushnumber(L, Minecraft::GetPlayerMoveSpeed());
            break;
        case hash("SwimSpeed"): {
            float vel;
            CTRPF::Process::ReadFloat(player_offsets::playerSwimSpeed, vel);
            lua_pushnumber(L, vel);
            break;
        }
        case hash("FlySpeed"):
            lua_pushnumber(L, Minecraft::GetFlightSpeed());
            break;
        case hash("CurrentHP"): 
            lua_pushnumber(L, Minecraft::GetCurrentHP());
            break;
        case hash("MaxHP"):
            lua_pushnumber(L, Minecraft::GetMaxHP());
            break;
        case hash("CurrentHunger"):
            //lua_pushnumber(L, Minecraft::GetCurrentHunger());
            {
                float retVal = 0.0f;
                u32 baseAddr = Minecraft::GetBaseAddress(Minecraft::Base::Status);
                if (baseAddr) {
                    u32 ptr = *(u32*)(baseAddr + 4 * 3);
                    u32 offset = ptr + 0xd0;
                    if (offset % 4 == 0 && offset > 0x100000) {
                        u32 ptrHungerBar;
                        CTRPF::Process::Read32(offset, ptrHungerBar);
                        u32 ptrHunger = ptrHungerBar + 0x60;
                        if (ptrHunger % 4 == 0 && ptrHunger > 0x100000)
                            CTRPF::Process::ReadFloat(ptrHunger, retVal);
                    }
                }
                lua_pushnumber(L, retVal);
            }
            break;
        case hash("MaxHunger"):
            //lua_pushnumber(L, Minecraft::GetMaxHunger());
            {
                float retVal = 0.0f;
                u32 baseAddr = Minecraft::GetBaseAddress(Minecraft::Base::Status);
                if (baseAddr) {
                    u32 ptr = *(u32*)(baseAddr + 4 * 3);
                    u32 offset = ptr + 0xd0;
                    if (offset % 4 == 0 && offset > 0x100000) {
                        u32 ptrHungerBar;
                        CTRPF::Process::Read32(offset, ptrHungerBar);
                        u32 ptrHunger = ptrHungerBar + 0x5c;
                        if (ptrHunger % 4 == 0 && ptrHunger > 0x100000)
                            CTRPF::Process::ReadFloat(ptrHunger, retVal);
                    }
                }
                lua_pushnumber(L, retVal);
            }
            break;
        case hash("CurrentLevel"):
            lua_pushnumber(L, Minecraft::GetCurrentLevel());
            break;
        case hash("LevelProgress"):
            lua_pushnumber(L, Minecraft::GetXPBarProgress());
            break;
        case hash("Gamemode"):
            lua_pushnumber(L, Minecraft::GetGameMode());
            break;
        case hash("ReachDistance"): {
            float distance;
            CTRPF::Process::ReadFloat(player_offsets::playerReachDistance, distance);
            lua_pushnumber(L, distance);
            break;
        }
        case hash("SprintDelay"):
            lua_pushnumber(L, Minecraft::GetSprintDelayTime());
            break;
        case hash("Dimension"):
            if (!ply)
                lua_pushnumber(L, 0);
            else
                lua_pushnumber(L, ply->dimId);
            break;
        default:
            valid_key = false;
            break;
    }
    
    return valid_key;
}

static int l_LocalPlayer_newindex(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return luaL_error(L, "Attempt to set unknown member of LocalPlayer");

    uint32_t key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    switch (key) {
        case hash("OnGround"):
            Minecraft::SetPlayerOnGround(lua_toboolean(L, 3));
            break;
        case hash("Sneaking"):
            Minecraft::SetPlayerSneaking(lua_toboolean(L, 3));
            break;
        case hash("Jumping"):
            Minecraft::SetPlayerJump(lua_toboolean(L, 3));
            break;
        case hash("Sprinting"):
            Minecraft::SetPlayerSprinting(lua_toboolean(L, 3));
            break;
        case hash("Flying"):
            Minecraft::SetPlayerFlying(lua_toboolean(L, 3));
            break;
        case hash("UnderWater"):
            Minecraft::SetPlayerUnderWater(lua_toboolean(L, 3));
            break;
        case hash("TouchingWall"):
            Minecraft::SetPlayerTouchingWall(lua_toboolean(L, 3));
            break;
        case hash("Invincible"):
            Minecraft::SetInvincible(lua_toboolean(L, 3));
            break;
        case hash("CanFly"):
            Minecraft::SetFlightAvailable(lua_toboolean(L, 3));
            break;
        case hash("CanConsumeItems"):
            Minecraft::SetItemConsumptionDisabled(!lua_toboolean(L, 3));
            break;
        case hash("BaseMoveSpeed"):
            Minecraft::SetBaseMoveSpeed(luaL_checknumber(L, 3));
            break;
        case hash("MoveSpeed"):
            Minecraft::SetPlayerMoveSpeed(luaL_checknumber(L, 3));
            break;
        case hash("SwimSpeed"):
            CTRPF::Process::WriteFloat(player_offsets::playerSwimSpeed, luaL_checknumber(L, 3));
            break;
        case hash("FlySpeed"):
            Minecraft::SetFlightSpeed(luaL_checknumber(L, 3));
            break;
        case hash("CurrentHP"):
            Minecraft::SetCurrentHP(luaL_checknumber(L, 3));
            break;
        case hash("MaxHP"):
            Minecraft::SetMaxHP(luaL_checknumber(L, 3));
            break;
        case hash("CurrentHunger"):
            //Minecraft::SetCurrentHunger(luaL_checknumber(L, 3));
            {
                u32 baseAddr = Minecraft::GetBaseAddress(Minecraft::Base::Status);
                if (baseAddr) {
                    u32 ptr = *(u32*)(baseAddr + 4 * 3);
                    u32 offset = ptr + 0xd0;
                    if (offset % 4 == 0 && offset > 0x100000) {
                        u32 ptrHungerBar;
                        CTRPF::Process::Read32(offset, ptrHungerBar);
                        u32 ptrHunger = ptrHungerBar + 0x60;
                        if (ptrHunger % 4 == 0 && ptrHunger > 0x100000) {
                            CTRPF::Process::WriteFloat(ptrHunger, luaL_checknumber(L, 3));
                        }
                    }
                }
            }
            break;
        case hash("MaxHunger"):
            //Minecraft::SetMaxHunger(luaL_checknumber(L, 3));
            {
                u32 baseAddr = Minecraft::GetBaseAddress(Minecraft::Base::Status);
                if (baseAddr) {
                    u32 ptr = *(u32*)(baseAddr + 4 * 3);
                    u32 offset = ptr + 0xd0;
                    if (offset % 4 == 0 && offset > 0x100000) {
                        u32 ptrHungerBar;
                        CTRPF::Process::Read32(offset, ptrHungerBar);
                        u32 ptrHunger = ptrHungerBar + 0x5c;
                        if (ptrHunger % 4 == 0 && ptrHunger > 0x100000) {
                            CTRPF::Process::WriteFloat(ptrHunger, luaL_checknumber(L, 3));
                        }
                    }
                }
            }
            break;
        case hash("CurrentLevel"):
            Minecraft::SetCurrentLevel(luaL_checknumber(L, 3));
            break;
        case hash("LevelProgress"):
            Minecraft::SetXPBarProgress(luaL_checknumber(L, 3));
            break;
        case hash("Gamemode"): {
            u8 gamemode = luaL_checknumber(L, 3);
            Minecraft::SetGameMode(gamemode);
            if (gamemode == 0)
                Minecraft::SetPlayerFlying(false);
            break;
        }
        case hash("ReachDistance"):
            CTRPF::Process::WriteFloat(player_offsets::playerReachDistance, luaL_checknumber(L, 3));
            break;
        case hash("SprintDelay"):
            Minecraft::SetSprintDelayTime(luaL_checknumber(L, 3));
            break;
        default:
            valid_key = false;
            break;
    }

    if (valid_key)
        return 0;
    else
        return luaL_error(L, "'%s' is not a valid member of LocalPlayer or is read-only value", lua_tostring(L, 2));
}

// ----------------------------------------------------------------------------

static inline void RegisterLocalPlayerMetatables(lua_State *L)
{
    luaL_newmetatable(L, "LocalPlayerMetatable");
    lua_pushcfunction(L, l_LocalPlayer_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_LocalPlayer_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pop(L, 1);
}

bool Core::Module::RegisterLocalPlayerModule(lua_State *L)
{
    RegisterLocalPlayerMetatables(L);

    lua_getglobal(L, "Game");
    lua_newtable(L); // LocalPlayer

    luaC_register_field(L, player_position_methods, "Position");
    luaC_register_field(L, player_velocity_methods, "Velocity");
    Core::Module::LocalPlayer::RegisterCameraModule(L);
    Core::Module::LocalPlayer::RegisterInventoryModule(L);
    
    luaC_setmetatable(L, "LocalPlayerMetatable"); // Set metatable at the end otherwise setfield doesn't work
    lua_setfield(L, -2, "LocalPlayer");
    lua_pop(L, 1);
    return true;
}
