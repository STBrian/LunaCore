#include "LuaModules.hpp"

#include <CTRPluginFramework.hpp>
#include "MC3DSPluginFramework.hpp"

#include "CoreGlobals.hpp"
#include "Core/Event.hpp"
#include "string_hash.hpp"
#include "Core/Scheduler.hpp"

#include "Enums.hpp"
#include "Helpers/LuaObject.hpp"

#include "game/Minecraft.hpp"
#include "Helpers/LuaCustomTable.hpp"

#include "MC3DSPluginFramework.hpp"
#include "Minecraft/Common/World/Level/Chunk/LevelChunk.hpp"

namespace CTRPF = CTRPluginFramework;
using namespace Core;

enum world_offsets : u32 {
    cloudsHeight = 0x3C5398
};

// ----------------------------------------------------------------------------

//$Game.World

// ----------------------------------------------------------------------------

/*
=Game.World.Loaded = false
=Game.World.Raining = false
=Game.World.Thunderstorm = false
=Game.World.CloudsHeight = 0.0
#---@type WeatherType
#Game.World.Weather = {}
*/

static int l_World_index(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;

    shash key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    MC3DSPluginFramework::BlockSource* blockSrc = nullptr;
    MC3DSPluginFramework::Level* level = nullptr;
    if (ply) {
        // blockSrc = ply->getBlo();
        level = &ply->getLevel();
    }

    switch (key) {
        case hash("Loaded"): // Read-only
            lua_pushboolean(L, GameState.WorldLoaded.load());
            break;
        case hash("Raining"):
            lua_pushboolean(L, level && level->weather()->get() == MC3DSPluginFramework::Weathers::Rain);
            break;
        case hash("Thunderstorm"):
            lua_pushboolean(L, level && level->weather()->get() == MC3DSPluginFramework::Weathers::Thunder);
            break;
        case hash("CloudsHeight"): {
            float value;
            CTRPF::Process::ReadFloat(world_offsets::cloudsHeight, value);
            lua_pushnumber(L, value);
            break;
        }
        case hash("Weather"):
            if (level) {
                MC3DSPluginFramework::WeatherState* weather = level->weather();
                Core::EnumGroup& group = EnumGroups::getInstance().getGroup("WeatherType");
                if (weather->get() & MC3DSPluginFramework::Weathers::Thunder)
                    LuaObjectUtils::NewObject(L, group.enumType, group.getItemByName("Thunder"));
                else if (weather->get() & MC3DSPluginFramework::Weathers::Rain)
                    LuaObjectUtils::NewObject(L, group.enumType, group.getItemByName("Rain"));
                else
                    LuaObjectUtils::NewObject(L, group.enumType, group.getItemByName("Clear"));
            } else {
                Core::EnumGroup& group = EnumGroups::getInstance().getGroup("WeatherType");
                LuaObjectUtils::NewObject(L, group.enumType, group.getItemByName("Clear"));
            }
            break;

        case hash("TargetBlock"):
            
            break;

        default:
            valid_key = false;
            break;
    }

    return valid_key;
}

static int l_World_newindex(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return LUACT_INVALID_KEY;

    shash key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    MC3DSPluginFramework::BlockSource* blockSrc = nullptr;
    MC3DSPluginFramework::Level* level = nullptr;
    // if (ply) {
    //     blockSrc = ply->GetBlockSource();
    //     level = ply->GetLevel();
    // }

    switch (key) {
        case hash("Raining"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            if (level) {
                const u32 weatherv = MC3DSPluginFramework::Weathers::Rain;
                u32 weather = lua_toboolean(L, 3) ? (level->weather()->get() | weatherv) : (level->weather()->get() | ~weatherv);
                level->weather()->set(weather);
            }
            break;
        case hash("Thunderstorm"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            if (level) {
                const u32 weatherv = MC3DSPluginFramework::Weathers::Thunder;
                u32 weather = lua_toboolean(L, 3) ? (level->weather()->get() | weatherv) : (level->weather()->get() | ~weatherv);
                level->weather()->set(weather);
            }
            break;
        case hash("CloudsHeight"): {
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            CTRPF::Process::WriteFloat(world_offsets::cloudsHeight, luaL_checknumber(L, 3));
            break;
        }
        case hash("Weather"): {
            Core::EnumItem* val = EnumItemUtils::LuaToEnumItemOrNull(L, 3, "WeatherType", true);
            if (!val) return LUACT_ERROR;
            if (level) level->weather()->set(val->value);
            break;
        }
        default:
            valid_key = false;
            break;
    }

    if (valid_key) return 0;
    else return LUACT_INVALID_KEY;
}

/*
- Displays an in-game message in the world screen if the player is currently inside a world. Returns true if success
## msg: string
## return: boolean
### Game.World.message
*/
static int l_World_message(lua_State *L) {
    const char* msg = luaL_checkstring(L, 1);
    if (MC3DSPluginFramework::SystemMessagesScreen::RunningInstance) {
        MC3DSPluginFramework::SystemMessagesScreen::RunningInstance->pushMessage(msg, false);
    }
    lua_pushboolean(L, MC3DSPluginFramework::SystemMessagesScreen::RunningInstance != nullptr);
    return 1;
}

/*
- Returns the entity that the player is looking at
## return: table?
### Game.World.getTargetEntity
*/
static int l_World_getTargetEntity(lua_State* L) {
    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    if (ply) {
        auto& level = ply->getLevel();
        u8 type = level.getHitResult().mType;
        if (type == MC3DSPluginFramework::HitResult::HitType::Entity) {
            MC3DSPluginFramework::Entity* entity = level.getHitResult().mEntity;
            lua_pushlightuserdata(L, entity);
        } else {
            lua_pushnil(L);
        }
    }
    return 0;
}

/*
- Returns the block that the player is looking at
## return: table?
### Game.World.getTargetBlock
*/
static int l_World_getTargetBlock(lua_State* L) {
    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    if (ply) {
        auto& level = ply->getLevel();
        u8 type = level.getHitResult().mType;
        if (type == MC3DSPluginFramework::HitResult::HitType::Block) {
            MC3DSPluginFramework::BlockPos blockPos = level.getHitResult().mBlockPos;
            lua_newtable(L);
            lua_pushnumber(L, blockPos.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, blockPos.y);
            lua_setfield(L, -2, "y");
            lua_pushnumber(L, blockPos.z);
            lua_setfield(L, -2, "z");
            return 1;
        }
    }
    return 0;
}

/*
- Sets the block at the provided coordinates
## x: integer
## y: integer
## z: integer
## blockId: integer
## blockData: integer
### Game.World.setBlock
*/
static void l_World_setBlock_impl(void* ud) {
    lua_State* L = (lua_State*)ud;
    int x = luaL_checknumber(L, 1);
    int y = luaL_checknumber(L, 2);
    int z = luaL_checknumber(L, 3);
    u16 blockId = luaL_checknumber(L, 4);
    u8 blockData = luaL_checknumber(L, 5);
    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    using BlockPos = MC3DSPluginFramework::BlockPos;
    using BlockData = MC3DSPluginFramework::BlockData;
    using BlockId = MC3DSPluginFramework::BlockId;
    using UpdateFlag = MC3DSPluginFramework::UpdateFlag;
    if (ply) {
        ply->getRegion().setBlockData(BlockPos(x, y, z), BlockData((BlockId)blockId, blockData), UpdateFlag::Default, ply);
    }
    // return 0;
}

static int l_World_setBlock(lua_State* L) {
    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    if (ply)
        // setBlockData requires to be executed in the game's MainThread
        Core::GameMainThreadScheduler::RunOnMainThreadAndWait(l_World_setBlock_impl, L);
    return 0;
}

/*
- Gets the properties of the block at the specified coordinates
## x: integer
## y: integer
## z: integer
## return: integer
## return: integer
### Game.World.getBlock
*/
static int l_World_getBlock(lua_State* L) {
    int x = luaL_checknumber(L, 1);
    int y = luaL_checknumber(L, 2);
    int z = luaL_checknumber(L, 3);
    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    using BlockPos = MC3DSPluginFramework::BlockPos;
    using BlockData = MC3DSPluginFramework::BlockData;
    if (ply) {
        BlockData data = ply->getRegion().getBlockIdAndData(BlockPos(x, y, z));
        lua_pushnumber(L, (u16)data.blockId);
        lua_pushnumber(L, data.dataId);
    } else {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
    }
    return 2;
}

/*
- Gets the current biome ID
## return: integer
### Game.World.getCurrentBiome
*/
static int l_World_getCurrentBiome(lua_State* L) {
    auto ply = MC3DSPluginFramework::Facade::getLocalPlayer();
    using BlockPos = MC3DSPluginFramework::BlockPos;
    using ChunkBlockPos = MC3DSPluginFramework::ChunkBlockPos;
    if (ply) {
        MC3DSPluginFramework::Vec3<float>& pos = ply->getPos();
        auto data = ply->getRegion().getLevelChunk(BlockPos(pos.x, pos.y, pos.z));
        if (data) {
            void* biome = data->getBiome(ChunkBlockPos(BlockPos(pos.x, pos.y, pos.z)));
            lua_pushnumber(L, *reinterpret_cast<u32*>((u32)biome + 0x90));
        } else 
            lua_pushnumber(L, -1);
    } else
        lua_pushnumber(L, -1);

    return 1;
}

#include "Minecraft/Common/Client/Sound/SoundEngine.hpp"

class SoundEngineProxy : public MC3DSPluginFramework::SoundEngine {
    public:
    virtual void playMusic(u32 soundId);
};

/*
- Plays a sound globally
## soundId: string
## volume: number
## pitch: number
### Game.World.playGlobalSound
*/
static int l_World_playGlobalSound(lua_State* L) {
    const char* soundId = luaL_checkstring(L, 1);
    float volume = luaL_checknumber(L, 2);
    float pitch = luaL_checknumber(L, 3);

    auto* game = MC3DSPluginFramework::Facade::getMinecraftGame();
    if (game) {
        auto engine = reinterpret_cast<MC3DSPluginFramework::gstd::unique_ptr<SoundEngineProxy>*>((u32)game + 0xDC);
        engine->get()->playMusic(hash(soundId));
        // game->playSound(hash(soundId), volume, pitch);
    }   
    return 0;
}

// ----------------------------------------------------------------------------

bool Core::Module::RegisterWorldModule(lua_State *L) {
    LuaCustomTable::RegisterNewCustomTable(L, "World", l_World_index, l_World_newindex);
    LuaCustomTable::GetIndexTable(L, "World");

    //$@@@Game.World.OnWorldJoin: EventClass
    Core::Event::NewEvent(L, "OnWorldJoin");

    //$@@@Game.World.OnWorldLeave: EventClass
    Core::Event::NewEvent(L, "OnWorldLeave");

    lua_pushcfunction(L, l_World_message);
    lua_setfield(L, -2, "message");
    lua_pushcfunction(L, l_World_getTargetEntity);
    lua_setfield(L, -2, "getTargetEntity");
    lua_pushcfunction(L, l_World_getTargetBlock);
    lua_setfield(L, -2, "getTargetBlock");
    lua_pushcfunction(L, l_World_setBlock);
    lua_setfield(L, -2, "setBlock");
    lua_pushcfunction(L, l_World_getBlock);
    lua_setfield(L, -2, "getBlock");
    lua_pushcfunction(L, l_World_getCurrentBiome);
    lua_setfield(L, -2, "getCurrentBiome");
    lua_pushcfunction(L, l_World_playGlobalSound);
    lua_setfield(L, -2, "playGlobalSound");

    lua_pop(L, 1); // Pop index table
    lua_getglobal(L, "Game");
    LuaCustomTable::SetNewCustomTable(L, -1, "World");
    lua_pop(L, 1); // Pop Game
    return true;
}