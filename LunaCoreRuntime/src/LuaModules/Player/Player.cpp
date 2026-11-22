#include "LuaModules.hpp"

#include <CTRPluginFramework.hpp>
#include "MC3DSPluginFramework.hpp"
#include "string_hash.hpp"

#include "game/Minecraft.hpp"
#include "game/memory.hpp"
#include "game/world/actor/player/Player.hpp"

#include "Helpers/LuaCustomTable.hpp"
#include "Helpers/LuaObject.hpp"

#include "Enums.hpp"

namespace CTRPF = CTRPluginFramework;

enum player_offsets : u32 {
    playerReachDistance = 0x100000 + 0x54FB68,
    playerSwimSpeed = 0x100000 + 0x3EA090,
};

enum player_fields_offsets : u32 {
    playerFieldMaxHunger = 0x188,
    playerFieldCurrentHunger = 0x18c,
};

static u32* getHungerBarOffset() {
    gstd::alloc_ptr<u32*> gPtr1(*reinterpret_cast<u32***>(0x918968));
    if (gPtr1) 
        return gstd::alloc_ptr<u32>(*(gPtr1 + 2)).get();
    return nullptr;
}

// ----------------------------------------------------------------------------

//!include LunaCoreRuntime/src/LuaModules/LoadModules.cpp

/* @doc:v2
@module Game.LocalPlayer
@desc Local player entity state, movement, and camera control
*/

// ----------------------------------------------------------------------------

/* @doc:v2
@module Game.LocalPlayer.Position
@desc Local player position management
*/

static MC3DSPluginFramework::Vec3<float> *safePlayerPositionGet() {
    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    if (ply) {
        return &ply->getPos();
    }
    return nullptr;
}

static MC3DSPluginFramework::Vec3<float> *safePlayerVelocityGet() {
    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    if (ply) {
        return &ply->getVelocity();
    }
    return nullptr;
}

/* @doc:v2
@func Game.LocalPlayer.Position.get(): number
@desc Gets local player position
@return number
@return number
@return number
*/
static int l_Player_Position_get(lua_State *L)
{
    auto pos = safePlayerPositionGet();
    if (pos) {
        lua_pushnumber(L, pos->x);
        lua_pushnumber(L, pos->y);
        lua_pushnumber(L, pos->z);
    } else {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
    }
    return 3;
}

/* @doc:v2
@func Game.LocalPlayer.Position.set(x: number, y: number, z: number)
@desc Sets player position
@param x number
@param y number
@param z number
*/
static int l_Player_Position_set(lua_State *L)
{
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float z = luaL_checknumber(L, 3);
    auto pos = safePlayerPositionGet();
    if (pos) {
        pos->x = x;
        pos->y = y;
        pos->z = z;
    }
    return 0;
}

/* @doc:v2
@func Game.LocalPlayer.Position.add(x: number, y: number, z: number)
@desc Adds X, Y, Z to current player position
@param x number
@param y number
@param z number
*/
static int l_Player_Position_add(lua_State *L)
{
    float x2 = luaL_checknumber(L, 1);
    float y2 = luaL_checknumber(L, 2);
    float z2 = luaL_checknumber(L, 3);
    auto pos = safePlayerPositionGet();
    if (pos) {
        pos->x += x2;
        pos->y += y2;
        pos->z += z2;
    }
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

/* @doc:v2
@module Game.LocalPlayer.Velocity
@desc Local player velocity management
*/

/* @doc:v2
@func Game.LocalPlayer.Velocity.get(): number
@desc Gets local player velocity
@return number
@return number
@return number
*/
static int l_Player_Velocity_get(lua_State *L)
{
    auto vel = safePlayerPositionGet();  
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (vel) {
        x = vel->x;
        y = vel->y;
        z = vel->z;
    }
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, z);
    return 3;
}

/* @doc:v2
@func Game.LocalPlayer.Velocity.set(x: number, y: number, z: number)
@desc Sets player velocity
@param x number
@param y number
@param z number
*/
static int l_Player_Velocity_set(lua_State *L)
{
    auto vel = safePlayerPositionGet();  
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float z = luaL_checknumber(L, 3);
    if (vel) {
        vel->x = x;
        vel->y = y;
        vel->z = z;
    }
    return 0;
}

/* @doc:v2
@func Game.LocalPlayer.Velocity.add(x: number, y: number, z: number)
@desc Adds X, Y, Z velocity to current player velocity
@param x number
@param y number
@param z number
*/
static int l_Player_Velocity_add(lua_State *L)
{
    auto vel = safePlayerPositionGet();  
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float z = luaL_checknumber(L, 3);
    if (vel) {
        vel->x += x;
        vel->y += y;
        vel->z += z;
    }
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

/* @doc:v2
@const Game.LocalPlayer.OnGround = false
@const Game.LocalPlayer.Sneaking = false
@const Game.LocalPlayer.Jumping = false
@const Game.LocalPlayer.Sprinting = false
@const Game.LocalPlayer.Flying = false
@const Game.LocalPlayer.UnderWater = false
@const Game.LocalPlayer.TouchingWall = false
@const Game.LocalPlayer.Invincible = false
@const Game.LocalPlayer.CanFly = false
@const Game.LocalPlayer.CanConsumeItems = false
@const Game.LocalPlayer.BaseMoveSpeed = 0.0
@const Game.LocalPlayer.MoveSpeed = 0.0
@const Game.LocalPlayer.SwimSpeed = 0.02
@const Game.LocalPlayer.FlySpeed = 0.0
@const Game.LocalPlayer.CurrentHP = 0.0
@const Game.LocalPlayer.MaxHP = 0.0
@const Game.LocalPlayer.CurrentHunger = 0.0
@const Game.LocalPlayer.MaxHunger = 0.0
@const Game.LocalPlayer.CurrentLevel = 0.0
@const Game.LocalPlayer.LevelProgress = 0.0
@const Game.LocalPlayer.Gamemode = 0
@const Game.LocalPlayer.ReachDistance = 0.0
@const Game.LocalPlayer.SprintDelay = 0.0
@const Game.LocalPlayer.Dimension = 0
@const Game.LocalPlayer.Loaded = false
*/
static int l_LocalPlayer_index(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;

    uint32_t key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();

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
        case hash("CurrentHunger"): {
            u32* ptrHungerBar = getHungerBarOffset();
            lua_pushnumber(L, ptrHungerBar ? *(float*)((u32)ptrHungerBar + playerFieldCurrentHunger) : 0.0f);
            break;
        }
        case hash("MaxHunger"): {
            u32* ptrHungerBar = getHungerBarOffset();
            lua_pushnumber(L, ptrHungerBar ? *(float*)((u32)ptrHungerBar + playerFieldMaxHunger) : 0.0f);
            break;
        }
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
            lua_pushnumber(L, ply ? ply->getLastDimensionId() : 0);
            break;
        case hash("Loaded"):
            lua_pushboolean(L, ply != NULL);
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
        return LUACT_INVALID_KEY;

    uint32_t key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    switch (key) {
        case hash("OnGround"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetPlayerOnGround(lua_toboolean(L, 3));
            break;
        case hash("Sneaking"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetPlayerSneaking(lua_toboolean(L, 3));
            break;
        case hash("Jumping"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetPlayerJump(lua_toboolean(L, 3));
            break;
        case hash("Sprinting"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetPlayerSprinting(lua_toboolean(L, 3));
            break;
        case hash("Flying"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetPlayerFlying(lua_toboolean(L, 3));
            break;
        case hash("UnderWater"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetPlayerUnderWater(lua_toboolean(L, 3));
            break;
        case hash("TouchingWall"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetPlayerTouchingWall(lua_toboolean(L, 3));
            break;
        case hash("Invincible"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetInvincible(lua_toboolean(L, 3));
            break;
        case hash("CanFly"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetFlightAvailable(lua_toboolean(L, 3));
            break;
        case hash("CanConsumeItems"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetItemConsumptionDisabled(!lua_toboolean(L, 3));
            break;
        case hash("BaseMoveSpeed"):
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            Minecraft::SetBaseMoveSpeed(lua_tonumber(L, 3));
            break;
        case hash("MoveSpeed"):
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            Minecraft::SetPlayerMoveSpeed(lua_tonumber(L, 3));
            break;
        case hash("SwimSpeed"):
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            CTRPF::Process::WriteFloat(player_offsets::playerSwimSpeed, lua_tonumber(L, 3));
            break;
        case hash("FlySpeed"):
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            Minecraft::SetFlightSpeed(lua_tonumber(L, 3));
            break;
        case hash("CurrentHP"):
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            Minecraft::SetCurrentHP(lua_tonumber(L, 3));
            break;
        case hash("MaxHP"):
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            Minecraft::SetMaxHP(lua_tonumber(L, 3));
            break;
        case hash("CurrentHunger"): {
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            u32* ptrHungerBar = getHungerBarOffset();
            if (ptrHungerBar)
                *(float*)((u32)ptrHungerBar + playerFieldCurrentHunger) = lua_tonumber(L, 3);
            break;
        }
        case hash("MaxHunger"): {
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            u32* ptrHungerBar = getHungerBarOffset();
            if (ptrHungerBar)
                *(float*)((u32)ptrHungerBar + playerFieldMaxHunger) = lua_tonumber(L, 3);
            break;
        }
        case hash("CurrentLevel"):
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            Minecraft::SetCurrentLevel(lua_tonumber(L, 3));
            break;
        case hash("LevelProgress"):
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            Minecraft::SetXPBarProgress(lua_tonumber(L, 3));
            break;
        case hash("Gamemode"): {
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            u8 gamemode = lua_tonumber(L, 3);
            Minecraft::SetGameMode(gamemode);
            if (gamemode == 0)
                Minecraft::SetPlayerFlying(false);
            break;
        }
        case hash("ReachDistance"):
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            CTRPF::Process::WriteFloat(player_offsets::playerReachDistance, lua_tonumber(L, 3));
            break;
        case hash("SprintDelay"):
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            Minecraft::SetSprintDelayTime(lua_tonumber(L, 3));
            break;
        default:
            valid_key = false;
            break;
    }

    if (valid_key) return 0; 
    else return LUACT_INVALID_KEY;
}

/* @doc:v2
@func Game.LocalPlayer.getState(stateId: EntityState): boolean
@desc Checks if the player is currently in the provided state. stateId: The entity state id from Game.Entity.States
@param stateId EntityState
@return boolean
*/
static int l_LocalPlayer_getState(lua_State* L) {
    using EntitySharedFlag = MC3DSPluginFramework::SynchedEntityData::EntitySharedFlag;
    Core::EnumItem* stateId = Core::EnumItemUtils::LuaToEnumItemOrNull(L, 1, "EntityState", true);
    if (!stateId) return lua_error(L);
    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    if (ply) lua_pushboolean(L, ply->getSharedFlag(static_cast<EntitySharedFlag>(stateId->value)));
    else lua_pushboolean(L, false);
    return 1;
}

// ----------------------------------------------------------------------------

bool Core::Module::RegisterLocalPlayerModule(lua_State *L)
{
    LuaCustomTable::RegisterNewCustomTable(L, "LocalPlayer", l_LocalPlayer_index, l_LocalPlayer_newindex);
    LuaCustomTable::GetIndexTable(L, "LocalPlayer");

    luaC_register_field(L, player_position_methods, "Position");
    luaC_register_field(L, player_velocity_methods, "Velocity");

    lua_pushcfunction(L, l_LocalPlayer_getState);
    lua_setfield(L, -2, "getState");

    Core::Module::LocalPlayer::RegisterCameraModule(L);
    Core::Module::LocalPlayer::RegisterInventoryModule(L);

    lua_pop(L, 1); // Pop LocalPlayer index table
    lua_getglobal(L, "Game");
    LuaCustomTable::NewCustomTable(L, "LocalPlayer");
    lua_setfield(L, -2, "LocalPlayer");
    lua_pop(L, 1);

    return true;
}
